#ifndef CUBOTMSGTRACKER_H
#define CUBOTMSGTRACKER_H

#include <QMap>

class MsgTrackData {
public:
    MsgTrackData(int m_id, int m_key);
    MsgTrackData(int m_id);
    MsgTrackData();

    bool isValid() const;

    int last_msg_id;
    QMap<int, int> key_id_map;
};

class CuBotMsgTrackerPrivate;

class CuBotMsgTracker
{
public:
    CuBotMsgTracker(int track_depth);

    ~CuBotMsgTracker();

    void addMsg(int chat_id, int msg_id);

    void addMsg(int chat_id, int msg_id, int key);

    int getMessageId(int chat_id, int key);

private:
    CuBotMsgTrackerPrivate *d;

    int m_removeOld(QMap<int, int> &map, int msg_id);
};

#endif // CUBOTMSGTRACKER_H
