#include "sourcehooks.h"
#include <sourcehook_impl.h>
#include <enginecallback.h>

using namespace SourceHook;

static Plugin g_PLID;
static Impl::CSourceHookImpl g_SourceHook;
ISourceHook* g_SHPtr = &g_SourceHook;

SH_DECL_HOOK2(IVEngineServer, UserMessageBegin, SH_NOATTRIB, 0, bf_write*, IRecipientFilter*, int);
SH_DECL_HOOK0_void(IVEngineServer, MessageEnd, SH_NOATTRIB, 0);

CHUDMsgInterceptor::CHUDMsgInterceptor()
{
	mUserMsgBeginHookId = SH_ADD_HOOK_MEMFUNC(IVEngineServer, UserMessageBegin,
		engine, this, &CHUDMsgInterceptor::OnUserMessageBegin, true);
	mMessageEndHookId = SH_ADD_HOOK_MEMFUNC(IVEngineServer, MessageEnd,
		engine, this, &CHUDMsgInterceptor::OnMessageEnd, false);
}

CHUDMsgInterceptor::~CHUDMsgInterceptor()
{
	SH_REMOVE_HOOK_ID(mUserMsgBeginHookId);
	SH_REMOVE_HOOK_ID(mMessageEndHookId);
}

void CHUDMsgInterceptor::Pause()
{
	g_SourceHook.PauseHookByID(mUserMsgBeginHookId);
	g_SourceHook.PauseHookByID(mMessageEndHookId);
}

void CHUDMsgInterceptor::Resume()
{
	g_SourceHook.UnpauseHookByID(mUserMsgBeginHookId);
	g_SourceHook.UnpauseHookByID(mMessageEndHookId);
}
