#ifndef SOURCEHOOKS_H
#define SOURCEHOOKS_H
#pragma once

class bf_write;
class IRecipientFilter;

class CHUDMsgInterceptor
{
	bf_write* OnUserMessageBegin(IRecipientFilter*, int);
	void OnMessageEnd();

	int mUserMsgBeginHookId, mMessageEndHookId;
	bf_write* mpWriter{};
	IRecipientFilter* mpRecipientFilter;

public:
	CHUDMsgInterceptor();

	~CHUDMsgInterceptor();

	void Pause();
	void Resume();
};

#endif // !SOURCEHOOKS_H
