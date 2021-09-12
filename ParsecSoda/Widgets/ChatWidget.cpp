#include "ChatWidget.h"

ChatWidget::ChatWidget(Hosting& hosting)
    : _hosting(hosting), _chatLog(hosting.getMessageLog()), _messageCount(0)
{
    setSendBuffer("\0");
}

bool ChatWidget::render()
{
    static Stopwatch stopwatch = Stopwatch(2000);
    stopwatch.start();

    AppStyle::pushTitle();
    ImGui::SetNextWindowSizeConstraints(ImVec2(300, 400), ImVec2(800, 900));
    ImGui::Begin("Chat", (bool*)0);
    AppStyle::pushInput();

    static ImVec2 size;
    size = ImGui::GetContentRegionAvail();

    ImVec2 cursor;

    static size_t index;
    index = 0;

    static vector<string>::iterator it;
    it = _chatLog.begin();
    ImGui::BeginChild("Chat Log", ImVec2(size.x, size.y - 160));
    for (; it != _chatLog.end(); ++it)
    {
        static float textHeight;
        cursor = ImGui::GetCursorPos();
        
        ImGui::TextWrapped((*it).c_str());
        textHeight = ImGui::GetCursorPosY() - cursor.y;

        ImGui::SetCursorPos(cursor);
        if (ImGui::Button(
            (string() + "### Chat Message " + to_string(index)).c_str(),
            ImVec2(size.x, textHeight)
        ))
        {
            toClipboard((*it));
            stopwatch.reset();
        }
        ++index;
    }
    if (_messageCount != _chatLog.size())
    {
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 10)
        {
            ImGui::SetScrollHereY(1.0f);
        }
        _messageCount = _chatLog.size();
    }
    ImGui::EndChild();

    ImGui::Separator();

    ImGui::BeginChild("Message Preview", ImVec2(size.x, 60));
    ImGui::TextWrapped(_previewBuffer);
    ImGui::EndChild();

    if (ImGui::IsWindowFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
        ImGui::SetKeyboardFocusHere(0);
    ImGui::SetNextItemWidth(size.x);
    try
    {
        strcpy_s(_lastBuffer, SEND_BUFFER_LEN, _sendBuffer);
    }
    catch (const std::exception&) {}
    if (ImGui::InputText(" ", _sendBuffer, SEND_BUFFER_LEN, ImGuiInputTextFlags_EnterReturnsTrue))
    {
        sendMessage();
    }
    if (strcmp(_sendBuffer, _lastBuffer) != 0)
    {
        string newSendBuffer = _sendBuffer;
        Stringer::replacePatternOnce(newSendBuffer, "%", "%%");

        string check = newSendBuffer.substr(0, SEND_BUFFER_LEN-1);

        try
        {
            strcpy_s(_previewBuffer, SEND_BUFFER_LEN, newSendBuffer.substr(0, SEND_BUFFER_LEN-1).c_str());
        }
        catch (const std::exception&) {}
    }

    cursor = ImGui::GetCursorPos();
    ImGui::Dummy(ImVec2(0, 5));
    if (_chatLog.size() > 0 && stopwatch.getRemainingTime() > 0)
    {
        static float fill = 1.0f;
        fill = (float)stopwatch.getRemainingTime() / stopwatch.getDuration();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, fill*fill), "Message copied.");
    }

    ImGui::SetCursorPos(cursor);

    ImGui::Indent(size.x - 50);
    
    if (ToggleIconButtonWidget::render(
        AppIcons::send, AppIcons::send, isDirty(),
        AppColors::primary, AppColors::alpha(AppColors::primary, 0.25f)
    ))
    {
        sendMessage();
    }

    AppStyle::pop();
    ImGui::End();
    AppStyle::pop();

    return true;
}

bool ChatWidget::isDirty()
{
    static size_t len;
    try
    {
        len = strnlen_s(_sendBuffer, SEND_BUFFER_LEN);
    }
    catch (const std::exception&)
    {
        len = 0;
    }
    return len > 0;
}


void ChatWidget::sendMessage()
{
    if (_hosting.isRunning() && strlen(_sendBuffer) > 0)
    {
        _hosting.sendHostMessage(_sendBuffer);
    }

    setSendBuffer("\0");
}

bool ChatWidget::setSendBuffer(const char* value)
{
    try
    {
        strcpy_s(_sendBuffer, SEND_BUFFER_LEN, value);
        strcpy_s(_lastBuffer, SEND_BUFFER_LEN, value);
        strcpy_s(_previewBuffer, SEND_BUFFER_LEN, value);
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }

    return false;
}

void ChatWidget::toClipboard(const string& message)
{
    string adjustedMessage = message;
    Stringer::replacePatternOnce(adjustedMessage, "%%", "%");

    OpenClipboard(0);
    EmptyClipboard();
    HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, adjustedMessage.size());
    if (!hg) {
        CloseClipboard();
        return;
    }
    memcpy(GlobalLock(hg), adjustedMessage.c_str(), adjustedMessage.size());
    GlobalUnlock(hg);
    SetClipboardData(CF_TEXT, hg);
    CloseClipboard();
    GlobalFree(hg);
}
