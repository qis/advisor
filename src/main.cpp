class Player {
public:
  Player()
  {
    player_.MediaEnded([this](auto, auto) { SetEvent(event_.get()); });
    player_.MediaFailed([this](auto, auto) { SetEvent(event_.get()); });
    player_.AutoPlay(true);
  }

  IAsyncAction Play(const Windows::Storage::Streams::IRandomAccessStream& stream)
  {
    player_.SetStreamSource(stream);
    co_await resume_on_signal(event_.get());
  }

private:
  MediaPlayer player_;
  handle event_{ check_pointer(CreateEvent(nullptr, FALSE, FALSE, nullptr)) };
};

class Synthesizer {
public:
  Synthesizer()
  {
    auto options = speech_synthesizer_.Options();
    options.AudioPitch(0.8);
    options.SpeakingRate(1.2);
    options.AppendedSilence(SpeechAppendedSilence::Min);
    options.PunctuationSilence(SpeechPunctuationSilence::Min);
  }

  template <typename String>
  auto Synthesize(String&& string)
  {
    return speech_synthesizer_.SynthesizeTextToStreamAsync(std::forward<String>(string));
  }

private:
  SpeechSynthesizer speech_synthesizer_;
};

class Window {
public:
  static constexpr PCWSTR title_text = TEXT(PROJECT_DESCRIPTION);
  static constexpr PCWSTR class_name = TEXT(PROJECT_DESCRIPTION PROJECT_VERSION);

  struct Step {
    std::size_t supply;
    std::string command;
    std::chrono::seconds time;
    SpeechSynthesisStream stream;
  };

  Window(int argc, char* argv[]) noexcept : hinstance_(GetModuleHandle(nullptr))
  {
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_NOCLOSE;
    wc.lpfnWndProc = Proc;
    wc.hInstance = hinstance_;
    wc.hIcon = wc.hIconSm = LoadIcon(hinstance_, MAKEINTRESOURCE(101));
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOWFRAME);
    wc.lpszClassName = class_name;
    RegisterClassEx(&wc);
    const auto ws = WS_CAPTION;
    const auto ex = WS_EX_APPWINDOW;
    CreateWindowEx(ex, class_name, title_text, ws, 0, 0, 640, 480, nullptr, nullptr, hinstance_, this);
    SetConsoleTitle((std::wstring(title_text) + L"   [CTRL+F12] Start/Stop   [CTRL+C] Exit").data());
  }

  Window(Window&& other) = delete;
  Window(const Window& other) = delete;
  Window& operator=(Window&& other) = delete;
  Window& operator=(const Window& other) = delete;

  ~Window()
  {
    UnregisterClass(class_name, hinstance_);
  }

  std::regex re{ R"((\d+)\s+(\d+):(\d+)\s+([^\s]+(?:\s+[^\s]+)*))", std::regex::ECMAScript | std::regex::optimize };

  IAsyncAction Advise()
  {
    const auto start = std::chrono::steady_clock::now();

    std::vector<Step> steps;
    if (auto is = std::ifstream("advisor.txt", std::ios::binary)) {
      for (std::string line; std::getline(is, line);) {
        std::smatch matches;
        if (std::regex_search(line, matches, re)) {
          // Get supply.
          std::size_t supply = 0;
          const auto ssupply = std::string(matches[1]);
          std::from_chars(ssupply.data(), ssupply.data() + ssupply.size(), supply);

          // Get command.
          const auto command = std::string(matches[4]);

          // Get time.
          const auto smin = matches[2].str();
          const auto ssec = matches[3].str();
          std::chrono::minutes::duration::rep min = 0;
          std::chrono::seconds::duration::rep sec = 0;
          std::from_chars(smin.data(), smin.data() + smin.size(), min);
          std::from_chars(ssec.data(), ssec.data() + ssec.size(), sec);
          const auto time = std::chrono::minutes(min) + std::chrono::seconds(sec);

          // Create stream.
          const auto text = /* ssupply + "! " + */ command;
          std::wstring wtext;
          const auto data = text.data();
          const auto size = static_cast<int>(text.size());
          wtext.resize(static_cast<size_t>(MultiByteToWideChar(CP_UTF8, 0, data, size, nullptr, 0)) + 1);
          wtext.resize(static_cast<size_t>(MultiByteToWideChar(CP_UTF8, 0, data, size, wtext.data(), size + 1)));
          auto stream = co_await synthesizer_.Synthesize(wtext);

          // Add step.
          steps.push_back({ supply, std::move(command), time, std::move(stream) });
        }
      }
    }

    if (steps.empty()) {
      std::cout << "Missing input." << std::endl;
      co_return;
    }
    std::cout << "Started: " << steps.size() << " Steps" << std::endl;
    messages_.Play(started_.value());

    auto token = co_await get_cancellation_token();
    for (const auto& e : steps) {
      const auto now = std::chrono::steady_clock::now();
      const auto sleep = e.time - (now - start) - std::chrono::seconds(2);

      // clang-format off
      const auto min = std::chrono::duration_cast<std::chrono::minutes>(e.time);
      const auto sec = std::chrono::duration_cast<std::chrono::seconds>(e.time) - min;
      std::cout << std::setfill('0') << std::setw(2) << min.count() << ':' << std::setw(2) << sec.count() << ' '
                << std::setfill(' ') << std::setw(3) << e.supply << ' ' << e.command << std::endl;
      // clang-format on

      if (min >= std::chrono::minutes(0) && sec >= std::chrono::seconds(0)) {
        if (token()) {
          co_return;
        }
        co_await resume_after{ std::chrono::duration_cast<TimeSpan>(sleep) };
      }
      if (token()) {
        co_return;
      }
      co_await player_.Play(e.stream);
    }

    std::cout << "Stopped!\r\n" << std::endl;
    messages_.Play(stopped_.value());
  }

  void OnCreate() noexcept
  {
    static HWND hwnd = nullptr;
    hwnd = hwnd_;

    try {
      started_.emplace(synthesizer_.Synthesize(L"Started.").get());
      stopped_.emplace(synthesizer_.Synthesize(L"Stopped.").get());
      aborted_.emplace(synthesizer_.Synthesize(L"Aborted.").get());
    }
    catch (const hresult_error& e) {
      std::wcerr << "Error: " << e.message().c_str() << "\r\n" << std::endl;
      PostQuitMessage(1);
      return;
    }

    SetConsoleCtrlHandler(
      [](DWORD type) -> BOOL {
        PostMessage(hwnd, WM_CLOSE, 0, 0);
        return TRUE;
      },
      TRUE);
    RegisterHotKey(hwnd_, 0, MOD_CONTROL | MOD_NOREPEAT, VK_F12);
  }

  void OnDestroy() noexcept
  {
    if (advise_) {
      advise_.Cancel();
    }
    UnregisterHotKey(hwnd_, 0);
    PostQuitMessage(0);
  }

  void OnHotkey() noexcept
  {
    try {
      if (advise_ && advise_.Status() == AsyncStatus::Started) {
        advise_.Cancel();
        std::cout << "Aborted!\r\n" << std::endl;
        messages_.Play(aborted_.value());
        return;
      }
      advise_ = Advise();
    }
    catch (const hresult_error& e) {
      if (e.code() != HRESULT_FROM_WIN32(ERROR_CANCELLED) && e.code() != E_ILLEGAL_METHOD_CALL) {
        std::wcerr << "Error " << e.code() << ": " << e.message().c_str() << "\r\n" << std::endl;
      }
    }
  }

  int Run() noexcept
  {
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
      DispatchMessage(&msg);
    }
    return static_cast<int>(msg.wParam);
  }

private:
  static LRESULT __stdcall Proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept
  {
    switch (msg) {
    case WM_CREATE:
      if (const auto window = reinterpret_cast<Window*>(reinterpret_cast<LPCREATESTRUCT>(lparam)->lpCreateParams)) {
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        window->hwnd_ = hwnd;
        window->OnCreate();
      }
      return 0;
    case WM_DESTROY:
      if (const auto window = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA))) {
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        window->OnDestroy();
      }
      return 0;
    case WM_HOTKEY:
      if (const auto window = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA))) {
        window->OnHotkey();
      }
      return 0;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
  }

  HINSTANCE hinstance_;
  HWND hwnd_ = nullptr;
  IAsyncAction advise_;

  Player player_;
  Player messages_;
  Synthesizer synthesizer_;
  std::optional<SpeechSynthesisStream> started_;
  std::optional<SpeechSynthesisStream> stopped_;
  std::optional<SpeechSynthesisStream> aborted_;
};

int main(int argc, char* argv[])
{
  init_apartment();
  Window window(argc, argv);
  return window.Run();
}
