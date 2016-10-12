int MDFND_NetworkConnect(void);
void NetplayText_InMainThread(uint8 *text, bool NetEcho);

int NetplayEventHook(const SDL_Event *event);
int NetplayEventHook_GT(const SDL_Event *event);

void Netplay_ToggleTextView(void);
int Netplay_GetTextView(void);
bool Netplay_IsTextInput(void);
bool Netplay_TryTextExit(void);

extern int MDFNDnetplay;


//
//
//

void Netplay_MT_Draw(const MDFN_PixelFormat& pformat, const int32 screen_w, const int32 screen_h);
