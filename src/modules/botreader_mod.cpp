#include "botreader_mod.h"

BotReaderModule::BotReaderModule()
{

}


int BotReaderModule::type() const
{
}

QString BotReaderModule::name() const
{
}

QString BotReaderModule::description() const
{

}

QString BotReaderModule::help() const
{
}

bool BotReaderModule::isVolatileOperation() const {
    return false;
}

int BotReaderModule::decode(const TBotMsg &msg)
{
}

bool BotReaderModule::process()
{
}

bool BotReaderModule::error() const
{
}

QString BotReaderModule::message() const
{
}
