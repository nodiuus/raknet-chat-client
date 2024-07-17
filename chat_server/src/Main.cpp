#include <RakNet\RakPeerInterface.h>
#include <RakNet\MessageIdentifiers.h>
#include <RakNet\BitStream.h>
#include <RakNet\PacketLogger.h>

#include <iostream>
#include <thread>

using namespace RakNet;

void packet_processor(RakPeerInterface*& server) {
    SystemAddress client_address = UNASSIGNED_SYSTEM_ADDRESS;

    while (true) {
        for (Packet* p = server->Receive(); p; server->DeallocatePacket(p), p = server->Receive()) {
            switch (p->data[0]) {
            case ID_NEW_INCOMING_CONNECTION:
                std::cout << "A connection is incoming.\n";
                client_address = p->systemAddress;
                {
                    std::cout << "Sending Connection Packet" << std::endl;
                    RakString rs;
                    BitStream bsOut;
                    bsOut.Write((MessageID)ID_USER_PACKET_ENUM);
                    bsOut.Write(RakString("Welcome!"));

                    server->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, p->systemAddress, false);
                }
                break;
            case ID_DISCONNECTION_NOTIFICATION:
                std::cout << "A client has disconnected.\n";
                break;
            case ID_CONNECTION_LOST:
                std::cout << "A client lost the connection.\n";
                break;
            case ID_USER_PACKET_ENUM:
            {
                BitStream bsIn(p->data, p->length, false);
                bsIn.IgnoreBytes(sizeof(MessageID));
                RakString rs;
                bsIn.Read(rs);
                std::cout << "Recieved: " << rs.C_String() << std::endl;

                BitStream bsOut;
                bsOut.Write((MessageID)ID_USER_PACKET_ENUM);
                bsOut.Write(rs);
                server->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
            }
            break;
            default:
                std::cout << "Received a message with ID: " << p->data[0] << "\n";
                break;
            }

            server->DeallocatePacket(p);
            p = server->Receive();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

int main() {
    RakPeerInterface* server = RakPeerInterface::GetInstance();
    SocketDescriptor sd(6000, "0.0.0.0"); //replace with different ip
    server->Startup(10, &sd, 1);
    server->SetMaximumIncomingConnections(10);
    std::cout << "Server started and listening on port 6000\n";

    std::thread process_packets(packet_processor, std::ref(server));
    process_packets.join();

    RakPeerInterface::DestroyInstance(server);
    return 0;
}