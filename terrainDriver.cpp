//////////////////////////////////////////////////////////////////////////////////////////////////
// 
// File: terrainDriver.cpp
// 
// Author: Frank Luna (C) All Rights Reserved
//
// System: AMD Athlon 1800+ XP, 512 DDR, Geforce 3, Windows XP, MSVC++ 7.0 
//
// Desc: Renders a terrain and allows you to walk around it. 
//          
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "d3dUtility.h"
#include "terrain.h"
#include "camera.h"
#include "fps.h"
#include "Terrian\resource.h"
#define TERRAIN_NUM 9
//
// Globals
//

IDirect3DDevice9* Device = 0; 

const int Width  = 640;
const int Height = 480;
int triIndex = -1;
Terrain* TheTerrain = 0;
//TODO:在这里改成fly模式
Camera   TheCamera(Camera::AIRCRAFT);
D3DXVECTOR3 lightDirection(0.0f, 1.0f, 0.0f);
FPSCounter* FPS = 0;

//
// Framework Functions
//
bool Setup()
{
	//
	// Create the terrain.
	//

	/* 
		加载地形，加载的时候读入地形高度，此时没有绘制。
	*/
	std::string FileNames[9];
	OPENFILENAME ofn;
	CHAR szOpenFileNames[80*MAX_PATH];
	CHAR szPath[MAX_PATH];
	CHAR szFileName[80*MAX_PATH];
    CHAR* p;
	int nLen = 0;
	ZeroMemory( &ofn, sizeof(ofn) );
	ofn.Flags = OFN_EXPLORER | OFN_ALLOWMULTISELECT;
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = szOpenFileNames;
	ofn.nMaxFile = sizeof(szOpenFileNames);
	ofn.lpstrFile[0] = '\0';
	ofn.lpstrFilter = TEXT("All Files(*.*)\0*.*\0");
	if( GetOpenFileName( &ofn ) )
	{  
		//把第一个文件名前的复制到szPath,即:
		//如果只选了一个文件,就复制到最后一个'\'
		//如果选了多个文件,就复制到第一个NULL字符
		lstrcpyn(szPath, szOpenFileNames, ofn.nFileOffset );
		//当只选了一个文件时,下面这个NULL字符是必需的.
		//这里不区别对待选了一个和多个文件的情况
		szPath[ ofn.nFileOffset ] = '\0';
		nLen = lstrlen(szPath);

		if( szPath[nLen-1] != '\\' )   //如果选了多个文件,则必须加上'\\'
		{
			lstrcat(szPath, TEXT("\\"));
		}

		p = szOpenFileNames + ofn.nFileOffset; //把指针移到第一个文件
		int i = 0;
		
		while( *p && i < TERRAIN_NUM)
		{   
			ZeroMemory(szFileName, sizeof(szFileName));
			lstrcpy(szFileName, szPath);  //给文件名加上路径  
			lstrcat(szFileName, p);    //加上文件名  
			std::string *tmpString = new std::string(szFileName);
			FileNames[i] = *tmpString;
			MessageBox(0,FileNames[i].c_str(),"title",0);
			i++;
			p += lstrlen(p) +1;     //移至下一个文件
		}
	}
	//到此为止之，没问题


	//问题应该出在构造函数这儿，其他算法都没大毛病，就是那个readrawFile!
	TheTerrain = new Terrain(Device, FileNames, 64 * 3 , 64 * 3, 5, 0.5f);
	
	TheTerrain->genTexture(&lightDirection);

	//
	// Create the font.
	//

	FPS = new FPSCounter(Device);

	//
	// Set texture filters.
	//

	Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	Device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

	//
	// Set projection matrix.
	//

	D3DXMATRIX proj;
	D3DXMatrixPerspectiveFovLH(
			&proj,
			D3DX_PI * 0.25f, // 45 - degree
			(float)Width / (float)Height,
			1.0f,
			1000.0f);
	Device->SetTransform(D3DTS_PROJECTION, &proj);

	return true;
}

void Cleanup()
{
	d3d::Delete<Terrain*>(TheTerrain);
	d3d::Delete<FPSCounter*>(FPS);
}

bool Display(float timeDelta)
{
	//
	// Update the scene:
	//

	if( Device )
	{
	if( ::GetAsyncKeyState('W') & 0x8000f )
			TheCamera.walk(100.0f * timeDelta);

		if( ::GetAsyncKeyState('S') & 0x8000f )
			TheCamera.walk(-100.0f * timeDelta);

		if( ::GetAsyncKeyState('A') & 0x8000f )
			TheCamera.strafe(-100.0f * timeDelta);

		if( ::GetAsyncKeyState('D') & 0x8000f )
			TheCamera.strafe(100.0f * timeDelta);

		if( ::GetAsyncKeyState('R') & 0x8000f )
			TheCamera.fly(100.0f * timeDelta);

		if( ::GetAsyncKeyState('F') & 0x8000f )
			TheCamera.fly(-100.0f * timeDelta);

		if( ::GetAsyncKeyState(VK_UP) & 0x8000f )
			TheCamera.pitch(1.0f * timeDelta);

		if( ::GetAsyncKeyState(VK_DOWN) & 0x8000f )
			TheCamera.pitch(-1.0f * timeDelta);

		if( ::GetAsyncKeyState(VK_LEFT) & 0x8000f )
			TheCamera.yaw(-1.0f * timeDelta);
			
		if( ::GetAsyncKeyState(VK_RIGHT) & 0x8000f )
			TheCamera.yaw(1.0f * timeDelta);

		if( ::GetAsyncKeyState('N') & 0x8000f )
			TheCamera.roll(1.0f * timeDelta);

		if( ::GetAsyncKeyState('M') & 0x8000f )
			TheCamera.roll(-1.0f * timeDelta);
	
		//这段代码是非飞行器模式下在地形中行走用到的，目前没用，注释掉。 
		/*
		D3DXVECTOR3 pos;
		TheCamera.getPosition(&pos);
		float height = TheTerrain->getHeight( pos.x, pos.z );
		pos.y = height + 5.0f; // add height because we're standing up
		TheCamera.setPosition(&pos);
		*/

		//更新摄像机的视角矩阵到设备视角矩阵。
		D3DXMATRIX V;
		TheCamera.getViewMatrix(&V);
		Device->SetTransform(D3DTS_VIEW, &V);

		//
		// Draw the scene:
		//

		Device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff000000, 1.0f, 0);
		Device->BeginScene();

		D3DXMATRIX I;
		D3DXMatrixIdentity(&I);

		//这段代码是用原来的The Terrain画
	    if( TheTerrain )
		{
			TheTerrain->draw(&I, true); 
		}
		if( FPS )
			FPS->render(0xffffffff, timeDelta);

		Device->EndScene();
		Device->Present(0, 0, 0, 0);
	}
	return true;
}
void setEditMenuRadio(HWND hwnd,int pos1,int begin,int end,int dest)
{
	//pos1:编辑下拉表中的位置
	HMENU hMenu = GetMenu(hwnd); //窗口主菜单
	HMENU subMenu1 = GetSubMenu(hMenu,1); //编辑

	HMENU subMenu2 = GetSubMenu(subMenu1,pos1);
	//subMenu2就是contain了
	CheckMenuRadioItem(subMenu2,begin,end,dest,MF_BYPOSITION);

}
bool getEditMenuCheckedState(HWND hwnd,int pos1,int dest)
{
	//第二个参数传过来的是INDENTIFIER
	MENUITEMINFO info;
	info.cbSize = sizeof(MENUITEMINFO);
	info.fMask = MIIM_STATE;
	//pos1:编辑下拉表中的位置
	HMENU hMenu = GetMenu(hwnd); //窗口主菜单
	HMENU subMenu1 = GetSubMenu(hMenu,1); //编辑

	HMENU subMenu2 = GetSubMenu(subMenu1,pos1);
	//subMenu2就是contain了
	GetMenuItemInfo(subMenu2,dest,true,&info);
	if (info.fState & MF_CHECKED )
		return true;
	else
		return false;
}
//
// WndProc
//
LRESULT CALLBACK d3d::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch( msg )
	{
	case WM_DESTROY:
		::PostQuitMessage(0);
		break;
		
	case WM_KEYDOWN:
		if( wParam == VK_ESCAPE )
			::DestroyWindow(hwnd);
		break;
	case WM_MOUSEMOVE:
		if ( wParam & MK_LBUTTON )
		{
			if(getEditMenuCheckedState(hwnd,1,0) ) //如果增加高度被选中了
				if ( triIndex != -1 )
				TheTerrain->changeHeight(triIndex);

			if(getEditMenuCheckedState(hwnd,1,1) ) //平滑
				if ( triIndex != -1 )
				TheTerrain->averageHeight(triIndex);
		}
		break;
	case WM_LBUTTONDOWN:
		triIndex = TheTerrain->pickTriangle(hwnd);
		break; 
	case WM_LBUTTONUP:
		triIndex = -1;
		OutputDebugString("triIndex = -1\n");
		break;
	case WM_COMMAND: //菜单消息响应
		switch(LOWORD(wParam))
		{
			//文件-退出
			case ID_40005:
				PostQuitMessage(0);
				break;
			//关于
			case ID_40009:
				MessageBox(0,"关于","About",0);
				break;

			//思路：用LoadMenu加载之后获得菜单句柄后set
			//编辑1-纹理0
			case ID_40007: //纹理0-选择纹理1 0
				setEditMenuRadio(hwnd,0,0,2,0);
				break;
			case ID_40008: //纹理0-选择纹理2 1
				setEditMenuRadio(hwnd,0,0,2,1);
				break;
			case ID_40010: //纹理0-不选择纹理 2
				setEditMenuRadio(hwnd,0,0,2,2);
				break;
			case ID_40012: //高度1-增加0
				setEditMenuRadio(hwnd,1,0,1,0);
				break;
			case ID_40013: //高度1-平滑1
				setEditMenuRadio(hwnd,1,0,1,1);
				break;
			//编辑1-物体2
			case ID_40015: //编辑1-物体2-增加0,range:0-3
				setEditMenuRadio(hwnd,2,0,3,0);
				break;
			case ID_40016://编辑1-物体2-删除1,range:0-3
				setEditMenuRadio(hwnd,2,0,3,1);
				break;
			case ID_40017://编辑1-物体2-移动2,range:0-3
				setEditMenuRadio(hwnd,2,0,3,2);
				break;
			case ID_40018://编辑1-物体2-不改变3,range:0-3
				setEditMenuRadio(hwnd,2,0,3,3);
				break;

		}
	}
	return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

//
// WinMain
//
int WINAPI WinMain(HINSTANCE hinstance,
				   HINSTANCE prevInstance, 
				   PSTR cmdLine,
				   int showCmd)
{
	if(!d3d::InitD3D(hinstance,
		Width, Height, true, D3DDEVTYPE_HAL, &Device))
	{
		::MessageBox(0, "InitD3D() - FAILED", 0, 0);
		return 0;
	}
		
	if(!Setup())
	{
		::MessageBox(0, "Setup() - FAILED", 0, 0);
		return 0;
	}

	d3d::EnterMsgLoop( Display );

	Cleanup();

	Device->Release();

	return 0;
}