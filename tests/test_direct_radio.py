"""Compile production radio packing, upload, queue and playback paths."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
text = (root / 'src/audio/oal/stream.cpp').read_text()

def extract(marker, prefix):
    first = text.index(marker)
    brace = text.index('{', first)
    last, depth = brace + 1, 1
    while depth:
        depth += (text[last] == '{') - (text[last] == '}')
        last += 1
    return prefix + text[first:last]

functions = '\n'.join((extract('CStream::FillBuffer(ALuint *alBuffer)', 'bool '),
                       extract('CStream::FillBuffers()', 'int32 '),
                       extract('CStream::SetPlay(bool state)', 'void '),
                       extract('CStream::BufferShouldBeFilledAndQueued(std::pair<ALuint, ALuint>* bufs)', 'bool '),
                       extract('CStream::QueueBuffers()', 'bool ')))
prelude = r'''
#include <cstdint>
#include <vector>
#include <map>
#include <utility>
#include <cstring>
#include <cassert>
#include <iostream>
using int16=int16_t;using uint8=uint8_t;using uint32=uint32_t;using int32=int32_t;
using ALuint=uint32_t;using ALint=int;using ALsizei=int;
enum { AL_NONE=0, AL_FORMAT_MONO16=1, AL_FORMAT_STEREO16=2, AL_SOURCE_STATE=3, AL_PLAYING=4, AL_STOPPED=5 };
#define NUM_STREAMBUFFERS 8
#include "StreamPcm.h"
struct Upload { int format,rate;std::vector<int16> pcm; };
static std::map<unsigned,Upload> uploads;
static std::map<unsigned,int> queues,plays;
bool alIsBuffer(unsigned b){return b!=0;}
void alBufferData(unsigned b,int format,const void *data,int bytes,int rate){
 auto p=(const int16*)data;uploads[b]={format,rate,std::vector<int16>(p,p+bytes/2)};
}
void alSourceQueueBuffers(unsigned source,int,const unsigned *){queues[source]++;}
void alGetSourcei(unsigned,int,int *value){*value=AL_STOPPED;}
void alSourcePlay(unsigned source){plays[source]++;}
void alSourceStop(unsigned){}
struct Decoder {
 std::vector<int16> pcm={100,-200,10000,31000};unsigned channels=2;
 uint32 Decode(void *p){memcpy(p,pcm.data(),pcm.size()*2);return pcm.size()*2;}
 uint32 GetChannels(){return channels;}uint32 GetSampleRate(){return 44100;}
};
struct CStream {
 bool m_bDirectPlayback=true,m_bActive=false;
 unsigned sources[2]={10,20};unsigned *m_pAlSources=sources;
 unsigned m_alBuffers[8]={1,2,3,4,5,6,7,8};
 int16 buffer[8];void *m_pBuffer=buffer;std::vector<int16> m_directPcm;
 Decoder decoder;Decoder *m_pSoundFile=&decoder;
 struct Pending {
  bool pending=true;
  bool peekPop(std::pair<ALuint,ALuint> *p){if(!pending)return false;pending=false;*p={1,0};return true;}
 } m_queueBuffers;
 bool HasSource(){return true;}bool IsOpened(){return true;}
 bool FillBuffer(unsigned *);int32 FillBuffers();void SetPlay(bool);
 bool BufferShouldBeFilledAndQueued(std::pair<ALuint,ALuint>*);bool QueueBuffers();
};
'''
tests = r'''
int main(){
 CStream radio;unsigned pair[2]={1,0};
 assert(radio.FillBuffer(pair));
 assert(uploads.size()==1 && uploads[1].format==AL_FORMAT_STEREO16 && uploads[1].rate==44100);
 assert((uploads[1].pcm==std::vector<int16>{100,10000,-200,31000}));
 assert(radio.FillBuffers()==4);assert(queues[10]==4 && queues[20]==0);
 radio.SetPlay(true);assert(plays[10]==1 && plays[20]==0);
 std::pair<ALuint,ALuint> recycled={1,0};
 assert(radio.BufferShouldBeFilledAndQueued(&recycled));assert(queues[10]==5 && queues[20]==0);
 assert(radio.QueueBuffers());assert(queues[10]==6 && queues[20]==0);
 radio.SetPlay(false);assert(!radio.m_bActive);
 radio.decoder.channels=1;radio.decoder.pcm={32767,-32768};assert(radio.FillBuffer(pair));
 assert((uploads[1].pcm==std::vector<int16>{32767,32767,-32768,-32768}));
 uploads.clear();queues.clear();plays.clear();
 CStream dialogue;dialogue.m_bDirectPlayback=false;unsigned nativePair[2]={1,2};
 assert(dialogue.FillBuffer(nativePair));assert(uploads.size()==2);
 assert(uploads[1].format==AL_FORMAT_MONO16 && uploads[2].format==AL_FORMAT_MONO16);
 assert((uploads[1].pcm==std::vector<int16>{100,-200}));
 assert((uploads[2].pcm==std::vector<int16>{10000,31000}));
 assert(dialogue.FillBuffers()==4);assert(queues[10]==4 && queues[20]==4);
 dialogue.SetPlay(true);assert(plays[10]==1 && plays[20]==1);
 std::vector<int16> output;int16 bad[3]={1,2,3};
 assert(!InterleaveStreamPcm(bad,3,2,output));assert(!InterleaveStreamPcm(bad,3,3,output));
 std::cout<<"PCM samples/rate intact; radio queues and plays one stereo source; other streams retain paired mono sources\n";
}
'''
with tempfile.TemporaryDirectory(prefix='re3-direct-radio-') as folder:
    folder = Path(folder)
    source = folder / 'radio.cpp'
    source.write_text(prelude + functions + tests)
    executable = folder / 'radio.exe'
    subprocess.run(['cl', '/nologo', '/EHsc', '/std:c++14', '/I' + str(root / 'src/audio/oal'), str(source), '/Fe:' + str(executable)], cwd=folder, check=True)
    subprocess.run([str(executable)], check=True)
