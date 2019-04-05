#ifndef CUMBIATELEGRAMDOC_H
#define CUMBIATELEGRAMDOC_H


/**
 * \mainpage
 *
 * \section title cumbia-telegram bot for telegram clients
 *
 * The cumbia-telegram bot is a small server employing the  <a href="https://telegram.org">Telegram</a>
 * <a href="https://core.telegram.org/api#bot-api">bot API</a> to establish a communication between an
 * *authorized user* and a control system managed by the
 * <a href="https://elettra-sincrotronetrieste.github.io/cumbia-libs/">cumbia libraries</a>.
 *
 *
 *
 * \section whatcando What is this for?
 * *cumbia-telegram bot* can be used to
 *
 * \li get customized notifications about the control system status
 * \li read any source supported by the *control system engines* supported and registered with *cumbia*
 *
 *
 * \par Modules and plugins
 *
 * The application is made up of *modules* and *plugins* that extend the bot functionality.
 *
 * Thanks to the utilities provided by *history* and *bookmark* plugins, most of the operations can be
 * executed again in a couple of taps.
 *
 * The *alias* plugin manages grants immediate access to the user's preferred actions and source names.
 *
 * The *last* plugin provides a shortcut to the last successful operation
 *
 * \subsection supported_engines Supported engines
 *
 * At the moment, the <a href="http://www.tango-controls.org">Tango</a> and <a href="https://epics.anl.gov/">EPICS</a>
 * control systems are seamlessly integrated into the application.
 * Sources from both engines can be combined into formulas.
 * Simple operators can be employed in formula expressions as
 * well as more complex *javascript* functions (see the \ref functions paragraph below).
 *
 * \subsection security Security
 *
 * Telegram is a cloud-based mobile and desktop messaging app with a focus on security and speed.
 * Nonetheless, as far as *bots* are concerned, no end to end encryption is applied between the parties.
 *
 * \subsubsection safe_csaccess Safe control system access
 *
 * \par Per user authorization
 * A small database stores *per user* authorization. A user joining the bot must be authorized by the
 * administrator before messaging.
 *
 * \par Per user limits
 * A table on the bot database is provided to establish limits for each user. For example, the number of
 * *monitors* i.e. periodic readers can be limited.
 *
 * \par Limits on the control system access
 * If the engine used by cumbia provides an event system for a given source, no restrictions are generally applied.
 * A *global* limit on the polling operations that are allowed in a given period of time is otherwise applied.
 * No matter how many *monitors* are polling concurrently, if the *global* polling interval is set to 1000 milliseconds,
 * the application guarantees that no more than one request per second is sent to any of the
 * registered control systems. The poll interval slots are equally divided amongst users and users running multiple
 * polling *monitors* are disadvantaged most.
 *
 *
 * \section functions Functions
 *
 *
 *
 *
 */
class CumbiaTelegramDoc {

};

#endif // CUMBIATELEGRAMDOC_H
