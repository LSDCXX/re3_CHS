"""Compile the production ordered stream mapping parser."""
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
code = r'''
#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include "AudioStreamMappings.h"
static bool exists(const char *name) { return strcmp(name,"audio/missing.wav")!=0; }
int main(int argc,char **argv) {
 char names[5][25] = {"AUDIO/A.OPUS","AUDIO/B.OPUS","AUDIO/C.OPUS","AUDIO/D.OPUS","AUDIO/E.OPUS"};
 bool mapped[5];
 FILE *f=tmpfile();assert(f);
 fputs("\xEF\xBB\xBF[Samples]\r\r\n; comment\n\nAUDIO\\A.WAV\nthis_filename_is_too_long_to_fit.wav\naudio/missing.wav\n../escape.wav\nAUDIO\\E.WAV\n[Other]\nwrong.wav\n",f);
 rewind(f);assert(ReadAudioStreamMappings(f,names,mapped,exists));fclose(f);
 assert(!strcmp(names[0],"AUDIO/A.WAV") && mapped[0]);
 assert(!strcmp(names[1],"AUDIO/B.OPUS") && !mapped[1]);
 assert(!strcmp(names[2],"AUDIO/C.OPUS") && !mapped[2]);
 assert(!strcmp(names[3],"AUDIO/D.OPUS") && !mapped[3]);
 assert(!strcmp(names[4],"AUDIO/E.WAV") && mapped[4]);
 assert(!ReadAudioStreamMappings(nullptr,names,mapped,exists));
 assert(!strcmp(names[0],"AUDIO/A.OPUS") && !mapped[0]);
 f=tmpfile();fputs("[samples]\nAUDIO/X.WAV",f);rewind(f);
 assert(ReadAudioStreamMappings(f,names,mapped,exists));fclose(f);
 assert(!strcmp(names[0],"AUDIO/X.WAV") && !strcmp(names[4],"AUDIO/E.OPUS"));
 if(argc>1) {
  char streams[196][25];bool mask[196];
  for(auto &entry:streams)strcpy(entry,"default.opus");
  f=fopen(argv[1],"rb");assert(f);
  assert(ReadAudioStreamMappings(f,streams,mask,exists));fclose(f);
  assert(!strcmp(streams[15],"AUDIO/JB.WAV"));
  assert(!strcmp(streams[16],"AUDIO/BET.WAV"));
  assert(!strcmp(streams[195],"AUDIO/cat1.WAV"));
 }
 std::cout<<"mapping order, CRCRLF/BOM, invalid slots, missing files, reload and defaults passed\n";
}
'''
with tempfile.TemporaryDirectory(prefix='re3-audio-mapping-') as directory:
    directory = Path(directory)
    source = directory / 'test.cpp'
    source.write_text(code, encoding='utf-8')
    executable = directory / 'test.exe'
    subprocess.run(['cl', '/nologo', '/EHsc', '/std:c++14', '/I' + str(root / 'src/audio'), str(source), '/Fe:' + str(executable)], cwd=directory, check=True)
    subprocess.run([str(executable)], check=True)

    # The user's archive is optional: copyrighted audio is not committed here.
    import zipfile
    archive = Path(sys.argv[1]) if len(sys.argv) > 1 else None
    if archive is not None:
        with zipfile.ZipFile(archive) as package:
            config = directory / 'audio-samples.ini'
            config.write_bytes(package.read('audio/audio-samples.ini'))
            entries = [line.strip() for line in config.read_text().splitlines() if line.strip() and not line.startswith('[')]
            assert len(entries) == 196
            subprocess.run([str(executable), str(config)], check=True)
