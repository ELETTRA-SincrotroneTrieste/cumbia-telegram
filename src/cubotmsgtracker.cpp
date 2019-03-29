#include "cubotmsgtracker.h"
#include <QMap>
#include <QMutableMapIterator>
#include <cumacros.h>
#include <QtDebug>

class CuBotMsgTrackerPrivate
{
public:
    QMap<int, MsgTrackData> msgmap;
    int track_depth;
};

// used by 3 parameter version of addMsg when map is empty for chat_id
MsgTrackData::MsgTrackData(int m_id, int m_key)
{
    last_msg_id = m_id;
    key_id_map[m_key] = m_id;
}

MsgTrackData::MsgTrackData(int m_id)
{
    last_msg_id = m_id;
}

MsgTrackData::MsgTrackData()
{
    last_msg_id = -1;
}

bool MsgTrackData::isValid() const
{
    return last_msg_id > -1;
}

CuBotMsgTracker::CuBotMsgTracker(int track_depth)
{
    d = new CuBotMsgTrackerPrivate;
    d->track_depth = track_depth;
}

CuBotMsgTracker::~CuBotMsgTracker()
{
    delete d;
}

void CuBotMsgTracker::addMsg(int chat_id, int msg_id)
{
    MsgTrackData &dl = d->msgmap[chat_id];
    if(!dl.isValid()) {
        printf("\e[0;36mCuBotMsgTracker::addMsg: chat_id %d saving msg_id %d\e[0m\n", chat_id, msg_id);
        d->msgmap[chat_id] = MsgTrackData(msg_id);
    }
    else {
        dl.last_msg_id = msg_id;
        if(!dl.key_id_map.isEmpty()) {
            QMutableMapIterator<int, int> it(dl.key_id_map);
            printf("\e[0;36mCuBotMsgTracker::addMsg: chat_id %d checking how older is every msg_id in map sized %d\e[0m...",
                   chat_id, dl.key_id_map.size());
            bool removed = m_removeOld(dl.key_id_map, msg_id);
            removed ?  printf("\e[1;31m REMOVED\e[0m\n") : printf(" \e[1;32m NOT REMOVED\e[0m\n");
        }
    }
}

void CuBotMsgTracker::addMsg(int chat_id, int msg_id, int key)
{
    printf("\e[0;34mCuBotMsgTracker::addMsg: chat_id %d addMsg msg_id %d KEY %d\e[0m\n", chat_id, msg_id, key);
    MsgTrackData &dl = d->msgmap[chat_id];
    if(!dl.isValid()) {
        printf("\e[0;34mCuBotMsgTracker::addMsg: chat_id %d saving msg_id %d\e[0m\n", chat_id, msg_id);
        d->msgmap[chat_id] = MsgTrackData(msg_id, key);
    }
    else {
        dl.key_id_map[key] = msg_id;
        bool removed = m_removeOld(dl.key_id_map, msg_id);
        removed > 0 ?  printf("\e[0;31m REMOVED %d \e[0m\n", removed) : printf(" \e[0;32m NOT REMOVED (size %d)\e[0m\n",
                                                                               dl.key_id_map.size());
    }
}

int CuBotMsgTracker::getMessageId(int chat_id, int key)
{
    MsgTrackData &dl = d->msgmap[chat_id];
    if(!dl.isValid() || !dl.key_id_map.contains(key)) {
        printf("\e[0;34mCuBotMsgTracker::getMessageId: chat_id %d returning -1 cuz either invalid (%d) or no key in key/id map (%d)\e[0m\n",
               chat_id, dl.isValid(), dl.key_id_map.contains(key));
        return -1;
    }
    // get latest msg_id for chat_id
    int last_msg_id = dl.last_msg_id;
    int msg_id_for_key = dl.key_id_map[key];
    if(last_msg_id - msg_id_for_key > d->track_depth) {
        printf("\e[0;34mCuBotMsgTracker::getMessageId:  chat_id %d returning -1 cuz entry for key %d too old (%d,"
               "last id is instead %d\e[0m\n", chat_id, key, msg_id_for_key, last_msg_id);
        return -1;
    }
    return msg_id_for_key;
}

int CuBotMsgTracker::m_removeOld(QMap<int, int>& map, int msg_id)
{
    int removed = 0;
    QMutableMapIterator<int, int> it(map);
    while(it.hasNext()) {
        it.next();
        const int m_id_in_map = it.value();
        if(msg_id - m_id_in_map > d->track_depth) { // too old: remove
            it.remove();
            removed++;
        }
    }
    return removed;
}
