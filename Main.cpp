#include "Menu.h"

int mainthread();
int emuThread();

static int LastTick = 0;

int main()
{
	//  load game data
	g_SOCOM = std::make_unique<SOCOM>();

	//	Initialize Menu
	g_Menu = std::make_unique<Menu>();

	//	Initialize d3d window
	g_dxWindow = std::make_unique<DxWindow>();
	g_dxWindow->Init();

	//	Initialize Background Thread
	//	std::thread wcw(mainthread);
	//	std::thread ecw(emuThread);

	while (g_Menu->bRunning)
	{
		bool bTimer = GetTickCount64() - LastTick > 500;

		// SHOW / HIDE MENU
		{
			if (GetAsyncKeyState(VK_RCONTROL) & 0x8000 && bTimer)
			{
				g_Menu->bShowMenu ^= 1;
				g_Menu->UpdateOverlayViewState(g_Menu->bShowMenu);
				switch (g_Menu->bShowMenu)
				{
				case(true): g_dxWindow->SetWindowFocus(g_dxWindow->GetWindowHandle()); break;
				case(false): g_dxWindow->SetWindowFocus(g_Memory.GetSocomInfo().hWnd); break;
				}

				LastTick = GetTickCount64();
			}
		}

		/* PCSX2 MEMORY UPDATE */
		{
			g_Memory.update();
		}

		/* SOCOM UPDATE */
		{
			g_SOCOM->Update();
		}

		/* DX WINDOW UPDATE */
		{
			g_dxWindow->CloneUpdate(g_Memory.GetSocomInfo().hWnd);
			g_dxWindow->Update(g_Menu->GetOverlay());
		}


		//	std::this_thread::sleep_for(1ms);
		std::this_thread::yield();
	}

	//	wcw.join();
	//	ecw.join();

	g_dxWindow->Shutdown();
	g_SOCOM->ShutDown();

	return EXIT_SUCCESS;
}

int mainthread()
{
	while (g_Menu->bRunning)
	{
		//	auto t0 = std::chrono::steady_clock::now();

		g_SOCOM->Update();

		//	g_Menu->m_refreshTimes[1] = std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - t0).count();

		//	std::this_thread::sleep_for(1ms);
		std::this_thread::yield();
	}

	return EXIT_SUCCESS;
}

int emuThread()
{

	while (g_Menu->bRunning)
	{
		//	auto t0 = std::chrono::steady_clock::now();

		g_Memory.update();

		//	g_Menu->m_refreshTimes[0] = std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - t0).count();

		//	std::this_thread::sleep_for(100ms);
		std::this_thread::yield();
	}

	return EXIT_SUCCESS;
}