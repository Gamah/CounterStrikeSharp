/*
 *  This file is part of CounterStrikeSharp.
 *  CounterStrikeSharp is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  CounterStrikeSharp is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with CounterStrikeSharp.  If not, see <https://www.gnu.org/licenses/>. *
 */
#include "UserMessage.h"

#include "networksystem/inetworkserializer.h"

#include <cstddef>

#include "core/gameconfig.h"

using namespace google;

namespace counterstrikesharp {
namespace netmessages {
// What the engine allocates per message since 1.41.9.7 (0x28 bytes, built by FindOrCreateNetMessage).
struct Info
{
    const char* name;           // a CUtlString: "<message> [<id>]"
    CNetMessage* (*allocate)(); // the allocator the message was registered with
    void* unk10;
    int messageId;
};
static_assert(offsetof(Info, allocate) == 0x8);
static_assert(offsetof(Info, messageId) == 0x18);

template <typename Arg> static INetworkMessageInternal* CallLookup(int slot, const char* slotName, Arg arg)
{
    if (slot < 0)
    {
        CSSHARP_CORE_ERROR("Missing gamedata offset {}", slotName);
        return nullptr;
    }
    using Fn = INetworkMessageInternal* (*)(INetworkMessages*, Arg);
    auto fn = reinterpret_cast<Fn>((*reinterpret_cast<void***>(globals::networkMessages))[slot]);
    return fn(globals::networkMessages, arg);
}

INetworkMessageInternal* FindPartial(const char* partialName)
{
    static const char* name = "INetworkMessages::FindNetworkMessagePartial";
    static int slot = globals::gameConfig->GetOffset(name);
    return CallLookup(slot, name, partialName);
}

INetworkMessageInternal* FindById(int messageId)
{
    static const char* name = "INetworkMessages::FindNetworkMessageById";
    static int slot = globals::gameConfig->GetOffset(name);
    return CallLookup(slot, name, messageId);
}

int GetId(INetworkMessageInternal* info) { return reinterpret_cast<Info*>(info)->messageId; }

std::string GetName(INetworkMessageInternal* info)
{
    const char* name = reinterpret_cast<Info*>(info)->name;
    if (name == nullptr) return {};

    // Strip the " [<id>]" the engine appends, so the name reads as it did before.
    std::string result(name);
    auto bracket = result.rfind(" [");
    if (bracket != std::string::npos && result.back() == ']') result.erase(bracket);
    return result;
}

CNetMessage* Allocate(INetworkMessageInternal* info) { return reinterpret_cast<Info*>(info)->allocate(); }
} // namespace netmessages

int UserMessage::GetMessageID() { return netmessages::GetId(msgSerializable); }

std::string UserMessage::GetMessageName() { return netmessages::GetName(msgSerializable); }

bool UserMessage::HasField(std::string fieldName)
{
    const google::protobuf::Descriptor* descriptor = msg->GetDescriptor();
    const google::protobuf::FieldDescriptor* field = descriptor->FindFieldByName(fieldName);

    if (field == nullptr || (field->label() == google::protobuf::FieldDescriptor::LABEL_REPEATED))
    {
        return false;
    }

    return this->msg->GetReflection()->HasField(*this->msg, field);
}

const CNetMessagePB<google::protobuf::Message>* UserMessage::GetProtobufMessage() { return msg; }
} // namespace counterstrikesharp
