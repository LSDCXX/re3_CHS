#pragma once
class CMenuManager;

// Native adaptation of LSDCXX/menu-map-iii (based on gennariarmando/menu-map).
// Uses re3 input, streaming and fonts; no ASI hooks or fixed game addresses.
class CMenuMap
{
public:
	static void Draw(CMenuManager &menu);
	static void Shutdown();
};
