#include <GLFW/glfw3.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/TextEditor.h>

//raknet i lvoe you raknet 
#include <RakNet/RakPeerInterface.h>
#include <RakNet/MessageIdentifiers.h>
#include <RakNet/BitStream.h>
#include <RakNet/PacketLogger.h>

#include <iostream>
#include <string>

using namespace RakNet;

#define SERVER_IP "0.0.0.0" //replace with ip for obvious reasons
#define SERVER_PORT 6000


void HandleMessages(RakPeerInterface* client, static ImGuiTextBuffer& chat_buffer, static bool& scroll_bottom) {
    RakNet::Packet* packet = nullptr;
    for (packet = client->Receive(); packet; client->DeallocatePacket(packet), packet = client->Receive()) {
        switch (packet->data[0]) {
        case ID_CONNECTION_REQUEST_ACCEPTED:
            chat_buffer.append("Connected to server.\n");
            break;
        case ID_CONNECTION_LOST:
        case ID_DISCONNECTION_NOTIFICATION:
            chat_buffer.append("Disconnected from server.\n");
            break;
        case ID_USER_PACKET_ENUM:
            BitStream bsIn(packet->data, packet->length, false);
            bsIn.IgnoreBytes(sizeof(MessageID));

            RakString message;
            bsIn.Read(message);

            //std::string mtom(message);

            chat_buffer.appendf("%s\n", message.C_String());
            scroll_bottom = true;
            break;
        }
    }
}

void Render(RakPeerInterface* client, static ImGuiTextBuffer &chat_buffer, std::string &username, static bool& scroll_bottom) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    static char input[256] = "";
    static bool open = false;

    ImGui::SetNextWindowSize(ImVec2(430, 400));
    ImGui::Begin("Chat Client", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    {
        ImGui::BeginChild("ChatRegion", ImVec2(0, ImGui::GetWindowHeight() - 75), false);
        ImGui::Text(chat_buffer.begin());

        if (scroll_bottom) {
            ImGui::SetScrollHereY(1.0f);
            scroll_bottom = false;
        }
        
        ImGui::EndChild();
    }
    ImGui::Separator();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::InputText("##input", input, IM_ARRAYSIZE(input), ImGuiInputTextFlags_EnterReturnsTrue)) {
        if (input[0]) {
            std::string combine = username + ": " + input;
            RakString rs;
            BitStream bsOut;
            bsOut.Write((MessageID)ID_USER_PACKET_ENUM);
            bsOut.Write(RakString(combine.c_str()));

            client->Send(&bsOut, HIGH_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_SYSTEM_ADDRESS, true);

            input[0] = '\0';
            scroll_bottom = true;
        }
    }
    ImGui::SetItemDefaultFocus();
    ImGui::SetKeyboardFocusHere(-1);
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void set_username(std::string &username) {
    std::cout << "Enter your username: ";
    getline(std::cin, username);
    if (username.size() >= 20) {
        std::cout << "Must be under 20 characters, try again!" << std::endl;
        set_username(username);
    }
}

int main(void)
{
    RakPeerInterface* client = RakNet::RakPeerInterface::GetInstance();
    RakNet::SocketDescriptor sd;
    client->Startup(1, &sd, 1);
    RakNet::ConnectionAttemptResult result = client->Connect(SERVER_IP, SERVER_PORT, nullptr, 0);

    if (result != RakNet::CONNECTION_ATTEMPT_STARTED) {
        printf("Failed to connect to server!\n");
    }
    else {
        printf("Connecting to server...\n");
    }

    std::string username;
    set_username(username);

    GLFWwindow* window;

    if (!glfwInit())
        return -1;

    window = glfwCreateWindow(640, 480, "", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    static bool scroll_bottom = false;

    while (!glfwWindowShouldClose(window))
    {
        static ImGuiTextBuffer chat_buffer;

        HandleMessages(client, chat_buffer, scroll_bottom);

        glClear(GL_COLOR_BUFFER_BIT);

        Render(client, chat_buffer, username, scroll_bottom);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}

