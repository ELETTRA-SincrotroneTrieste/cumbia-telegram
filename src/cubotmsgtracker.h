#ifndef CUBOTMSGTRACKER_H
#define CUBOTMSGTRACKER_H

#include <QMap>

class MsgTrackData {
public:
    MsgTrackData(int m_id, int m_key);
    MsgTrackData(int m_id);
    MsgTrackData();

    bool isValid() const;

    QMap<int, int> key_id_map;
};

class CuBotMsgTrackerPrivate;

class CuBotMsgTracker
{
public:
    CuBotMsgTracker();

    ~CuBotMsgTracker();

    void addMsg(int chat_id, int msg_id, int key, int depth);

    int getMessageId(int chat_id, int key, int depth);

private:
    CuBotMsgTrackerPrivate *d;

    int m_removeOld(QMap<int, int> &map, int msg_id, int depth);
};

#endif // CUBOTMSGTRACKER_H
