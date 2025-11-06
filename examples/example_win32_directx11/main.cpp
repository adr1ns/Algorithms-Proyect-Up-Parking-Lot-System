// Dear ImGui: standalone example application for Windows API + DirectX 11

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include <string>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>

// Data
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Variables
bool done = false;
bool opened = true; // Sets the state of the window displayed
bool parkCarAction = false; // Sets the state of the child window
bool takeCarAction = false; // Sets the state of the child window
bool smallCarParking[20] = {};
bool bigCarParking[8] = {};
bool fullParking = false;
bool advancedCarParking = false;

// Main code
int main(int, char**)
{
    // Make process DPI aware and obtain main monitor scale
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    // Create application window
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Estacionamiento UP", WS_OVERLAPPEDWINDOW, 100, 100, (int)(1280 * main_scale), (int)(800 * main_scale), nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details. If you like the default font but want it to scale better, consider using the 'ProggyVector' from the same author!
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop




    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window being minimized or screen locked
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();


        // --------------------------------------------------------------------------------------------
        // This is the code that we actually wrote
        ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_Always); // Set the size of the Main Window
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        if (ImGui::Begin("Sistema Estacionamiento UP", &opened, ImGuiWindowFlags_NoResize || ImGuiWindowFlags_NoMove || ImGuiWindowFlags_NoCollapse)) { // Starts the first window

            if (ImGui::Button("Estacionar Carro (Recomendado)", ImVec2(300, 50))) { // Button to open Park Car menu
                takeCarAction = false; // Close the other menu if open
                advancedCarParking = false; // Close the other menu if open
                parkCarAction = true; // Opens the menu
            }
            ImGui::SameLine();
            if (ImGui::Button("Estacionar Carro Avanzado", ImVec2(300, 50))) {
                takeCarAction = false; // Close the other menu if open
                parkCarAction = false; // Close the other menu if open
                advancedCarParking = true; // Opens the advance car parking menu
            }
            ImGui::SameLine();
            if (ImGui::Button("Retirar Carro", ImVec2(300, 50))) { // Button to open Take Car menu
                parkCarAction = false; // Close the other menu if open
                advancedCarParking = false; // Close the other menu if open
                takeCarAction = true; // Opens the menu
            }
            ImGui::SameLine();
            if (ImGui::Button("Finalizar")) { // Button to finish program
                done = true; // Sets the value of done to true, ending the while loop
            }

            if (parkCarAction == true) { // Menu for the parking Car Act90j
                if (ImGui::BeginChild("Estacionar Carro", ImVec2(1280, 600), true)) {
                    ImGui::Text("Que categoria es tu carro?");
                    if (ImGui::Button("Pequeno", ImVec2(500, 500))) {
                        for (int i = 0; i < 20; i++) {
                            if (smallCarParking[i] == 0) {
                                smallCarParking[i] = 1;
                                parkCarAction = false; // Closes the Park Car Window
                                break; //Ends the for
                            }
                        } // Check if small spaces available
                        if (parkCarAction == true) {
                            for (int i = 0; i < 8; i++) {
                                if (bigCarParking[i] == 0) {
                                    bigCarParking[i] = 1;
                                    parkCarAction = false; // Closes the Park Car Window
                                    break; //Ends the for
                                }
                                else if (bigCarParking[7] == 1) {
                                    fullParking = true;  // Turns on the variable telling that the parking is full
                                    parkCarAction = false; // Closes the Park Car Window
                                    break; //Ends the for
                                }
                            }

                        }
                    }
                    ImGui::SameLine(); // Puts the two buttons next to each other
                    if (ImGui::Button("Grande", ImVec2(500, 500))) { // Button for big cars
                        for (int i = 0; i < 8; i++) {
                            if (bigCarParking[i] == 0) {
                                bigCarParking[i] = 1;
                                parkCarAction = false; // Closes the Park Car Window
                                break; //Ends the for
                            } else if (bigCarParking[7] == 1) {
                                fullParking = true;  // Turns on the variable telling that the parking is full
                                parkCarAction = false; // Closes the Park Car Window
                            }
                        }
                    }
                    if (ImGui::Button("Cerrar Ventana")) {
                        parkCarAction = false; // Closes the Park Car Windows
                    }
                } ImGui::EndChild();
            }

            if (advancedCarParking == true) {
                if (ImGui::BeginChild("Estacionar carro avanzado", ImVec2(1280, 600), true)) {
                    ImGui::Text("Plazas pequenas");
                    for (int i = 1; i <= 20; i++) {
                        std::string carNumber = "Carrito\n" + std::to_string(i); // Converts the into to a string
                        std::string emptyCarNumber = "Libre\nPequeno\n" + std::to_string(i); // Converts the into to a string
                        if (smallCarParking[i - 1] == 1) {
                            ImGui::BeginDisabled(); // Starts the next elements as disabled, not being abled to be interacted with
                            if (ImGui::Button(carNumber.c_str(), ImVec2(100, 50))) {

                            }ImGui::EndDisabled(); // Ends the block for all the interactivity
                        }
                        else {
                            if (ImGui::Button(emptyCarNumber.c_str(), ImVec2(100, 50))) {
                                smallCarParking[i - 1] = 1; // Returns the space to available
                                
                            }
                        }
                        if (i % 10 == 0) {
                            ImGui::NewLine(); // If there is a group of 10, enter a space
                        }
                        else {
                            ImGui::SameLine(); // Puts the two buttons next to each other
                        }

                    }
                    ImGui::NewLine();
                    ImGui::Text("Plazas Grandes");
                    for (int j = 1; j <= 8; j++) {
                        std::string carNumber = "Carrote\n" + std::to_string(j); // Converts the into to a string
                        std::string emptyCarNumber = "Libre\nGrande\n" + std::to_string(j); // Converts the into to a string
                        if (bigCarParking[j - 1] == 1) {
                            ImGui::BeginDisabled(); // Starts the next elements as disabled, not being abled to be interacted with
                            if (ImGui::Button(carNumber.c_str(), ImVec2(200, 100))) {
                                

                            }ImGui::EndDisabled(); // Ends the block for all the interactivity
                        }
                        else {
                            if (ImGui::Button(emptyCarNumber.c_str(), ImVec2(200, 100))) {
                                bigCarParking[j - 1] = 1; // Returns the space to available
                                
                            }
                        }
                        if (j % 5 == 0) {
                            ImGui::NewLine();
                        }
                        else {
                            ImGui::SameLine();
                        }

                    }
                    ImGui::NewLine();
                    if (ImGui::Button("Cerrar Ventana")) {
                        advancedCarParking = false; // Closes the Park Car Windows
                    }
                } ImGui::EndChild();
            }
        

                if (takeCarAction == true) {
                    if (ImGui::BeginChild("Retirar Carro", ImVec2(1280, 600), true)) {
                        ImGui::Text("Plazas pequenas");
                        for (int i = 1; i <= 20; i++) {
                            std::string carNumber = "Carrito\n" + std::to_string(i); // Converts the into to a string
                            std::string emptyCarNumber = "Libre\nPequeno\n" + std::to_string(i); // Converts the into to a string
                            if (smallCarParking[i-1] == 0) {
                                ImGui::BeginDisabled(); // Starts the next elements as disabled, not being abled to be interacted with
                                if (ImGui::Button(emptyCarNumber.c_str(), ImVec2(100, 50))) {
                                    

                                }ImGui::EndDisabled(); // Ends the block for all the interactivity
                            }
                            else {
                                if (ImGui::Button(carNumber.c_str(),  ImVec2(100, 50))) {
                                    smallCarParking[i-1] = 0; // Returns the space to available
                                    
                                }
                            }
                            if (i % 10 == 0) {
                                ImGui::NewLine(); // If there is a group of 10, enter a space
                            } else {
                                ImGui::SameLine(); // Puts the two buttons next to each other
                            }
                            
                        }
                        ImGui::NewLine();
                        ImGui::Text("Plazas Grandes");
                        for (int j = 1; j <= 8; j++) {
                            std::string carNumber = "Carrote\n" + std::to_string(j); // Converts the into to a string
                            std::string emptyCarNumber = "Libre\nGrande\n" + std::to_string(j); // Converts the into to a string
                            if (bigCarParking[j - 1] == 0) {
                                ImGui::BeginDisabled(); // Starts the next elements as disabled, not being abled to be interacted with
                                if (ImGui::Button(emptyCarNumber.c_str(), ImVec2(200, 100))) {
                                    

                                }ImGui::EndDisabled(); // Ends the block for all the interactivity
                            }
                            else {
                                if (ImGui::Button(carNumber.c_str(), ImVec2(200, 100))) {
                                    bigCarParking[j - 1] = 0; // Returns the space to available
                                    
                                }
                            }
                            if (j % 5 == 0) {
                                ImGui::NewLine();
                            }
                            else {
                                ImGui::SameLine();
                            }

                        }
                        ImGui::NewLine();
                        if (ImGui::Button("Cerrar Ventana")) {
                            takeCarAction = false; // Closes the Park Car Windows
                        }
                    } ImGui::EndChild();
                }
            



                if (fullParking == true) { // Starts the menu that the parking is full
                    if (ImGui::BeginChild("Estacionamiento Lleno", ImVec2(1280, 800))) {
                        ImGui::Text("El Estacionamiento esta lleno.\nPor favor, espera a que se vacie un lugar");
                        if (ImGui::Button("Cerrar Ventana")) {
                            fullParking = false; // Closes the window
                        }
                    }ImGui::EndChild();
                }



        } ImGui::End(); // Does a clean up after ending the program

    // __________________________________________________________________________________________________________________

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present
        HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
        //HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions




bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
