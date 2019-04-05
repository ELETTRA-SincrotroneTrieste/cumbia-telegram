## cumbia-telegram bot

Sign in the *cumbia-telegram* bot to chat with a [Tango](http://www.tango-controls.org) or [EPICS](https://epics.anl.gov/)
control system from *everywhere*.

With the *cumbia-telegram* bot you can read values from the aforementioned control systems, monitor them
and receive alerts when something special happens. You text the bot, the bot texts you back.
It's simple, fast, intuitive and it's *everywhere*. A telegram account and a client are all you need to
start. [Telegram](https://telegram.org/) is a  cloud-based mobile and desktop messaging app focused
on security and speed. It is available for *Android*, *iPhone/iPad*, *Windows*, *macOS*, *Linux*... as
well as a *web application*.

On the *server side*, *cumbia-telegram* provides the administrator full control over the allocation of resources,
the network load and the people authorized to chat with the bot. Additionally, the access on the systems is
*read only*.


Parameters

--db=/path/to/sqliteDB.sql (a file where the sqlite db will be stored)

--token=telegram_bot_token (as given by botfather at bot creation)

EXAMPLE

./bin/cumbia-telegram --token=bot635922604:AAEgG6db_3kkzYZqh-LBxi-ubvl5UIEW7gE --db=/tmp/botdb.dat
