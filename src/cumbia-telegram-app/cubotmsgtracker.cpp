#include "cubotmsgtracker.h"
#include <QMap>
#include <QMutableMapIterator>
#include <cumacros.h>
#include <QtDebug>

class CuBotMsgTrackerPrivate
{
public:
    QMap<int, MsgTrackData> msgmap;
    int last_msg_id;
};

// used by 3 parameter version of addMsg when map is empty for chat_id
MsgTrackData::MsgTrackData(int m_id, int m_key)
{
    key_id_map[m_key] = m_id;
}

MsgTrackData::MsgTrackData()
{

}

bool MsgTrackData::isValid() const
{
    return key_id_map.size() > 0;
}

CuBotMsgTracker::CuBotMsgTracker()
{
    d = new CuBotMsgTrackerPrivate;
    d->last_msg_id = -1;
}

CuBotMsgTracker::~CuBotMsgTracker()
{
    delete d;
}

void CuBotMsgTracker::addMsg(int chat_id, int msg_id, int key, int depth)
{
    printf("\e[0;34mCuBotMsgTracker::addMsg: %p chat_id %d addMsg msg_id %d KEY %d DEPTH %d\e[0m\t", this, chat_id, msg_id, key, depth);
    if(msg_id > d->last_msg_id)
        d->last_msg_id = msg_id;
    MsgTrackData &dl = d->msgmap[chat_id];
    if(!dl.isValid()) {
        // create MsgTrackData for chat_id regardless the key value
        d->msgmap[chat_id] = MsgTrackData(msg_id, key);
        dl = d->msgmap[chat_id];
    }
    // now dl is valid
    if(key > -1)
        dl.key_id_map[key] = msg_id;

    bool removed = m_removeOld(dl.key_id_map, msg_id, depth);
    removed > 0 ?  printf("\e[0;31m REMOVED %d \e[0m\n", removed) : printf(" \e[0;32m NOT REMOVED (size %d)\e[0m\n",
                                                                           dl.key_id_map.size());
}

int CuBotMsgTracker::getMessageId(int chat_id, int key, int depth)
{
    MsgTrackData &dl = d->msgmap[chat_id];
    if(!dl.isValid() || !dl.key_id_map.contains(key)) {
        printf("\e[0;36mCuBotMsgTracker::getMessageId: chat_id %d returning -1 cuz either invalid (%d) or no key in key/id map (%d)\e[0m\n",
               chat_id, dl.isValid(), dl.key_id_map.contains(key));
        return -1;
    }
    // get latest msg_id for chat_id
    int msg_id_for_key = dl.key_id_map[key];
    printf("\e[0;36mCuBotMsgTracker::getMessageId:  chat_id %d  key %d msg_id_for_key %d last msg id %d (D=%d) DEPTH %d\e[0m\n",
          chat_id, key, msg_id_for_key, d->last_msg_id, d->last_msg_id - msg_id_for_key, depth);
    if(d->last_msg_id - msg_id_for_key >= depth) {
        printf("\e[0;36mCuBotMsgTracker::getMessageId:  chat_id %d returning -1 cuz entry for key %d too old (%d,"
               "last id is instead %d\e[0m\n", chat_id, key, msg_id_for_key, d->last_msg_id);
        return -1;
    }
    return msg_id_for_key;
}

int CuBotMsgTracker::m_removeOld(QMap<int, int>& map, int msg_id, int depth)
{
    int removed = 0;
    QMutableMapIterator<int, int> it(map);
    while(it.hasNext()) {
        it.next();
        const int m_id_in_map = it.value();
        if(msg_id - m_id_in_map > depth) { // too old: remove
            it.remove();
            removed++;
        }
    }
    return removed;
}
