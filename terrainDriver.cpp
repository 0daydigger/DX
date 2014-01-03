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
//TODO:������ĳ�flyģʽ
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
		���ص��Σ����ص�ʱ�������θ߶ȣ���ʱû�л��ơ�
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
		//�ѵ�һ���ļ���ǰ�ĸ��Ƶ�szPath,��:
		//���ֻѡ��һ���ļ�,�͸��Ƶ����һ��'\'
		//���ѡ�˶���ļ�,�͸��Ƶ���һ��NULL�ַ�
		lstrcpyn(szPath, szOpenFileNames, ofn.nFileOffset );
		//��ֻѡ��һ���ļ�ʱ,�������NULL�ַ��Ǳ����.
		//���ﲻ����Դ�ѡ��һ���Ͷ���ļ������
		szPath[ ofn.nFileOffset ] = '\0';
		nLen = lstrlen(szPath);

		if( szPath[nLen-1] != '\\' )   //���ѡ�˶���ļ�,��������'\\'
		{
			lstrcat(szPath, TEXT("\\"));
		}

		p = szOpenFileNames + ofn.nFileOffset; //��ָ���Ƶ���һ���ļ�
		int i = 0;
		
		while( *p && i < TERRAIN_NUM)
		{   
			ZeroMemory(szFileName, sizeof(szFileName));
			lstrcpy(szFileName, szPath);  //���ļ�������·��  
			lstrcat(szFileName, p);    //�����ļ���  
			std::string *tmpString = new std::string(szFileName);
			FileNames[i] = *tmpString;
			MessageBox(0,FileNames[i].c_str(),"title",0);
			i++;
			p += lstrlen(p) +1;     //������һ���ļ�
		}
	}
	//����Ϊֹ֮��û����


	//����Ӧ�ó��ڹ��캯������������㷨��û��ë���������Ǹ�readrawFile!
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
	
		//��δ����ǷǷ�����ģʽ���ڵ����������õ��ģ�Ŀǰû�ã�ע�͵��� 
		/*
		D3DXVECTOR3 pos;
		TheCamera.getPosition(&pos);
		float height = TheTerrain->getHeight( pos.x, pos.z );
		pos.y = height + 5.0f; // add height because we're standing up
		TheCamera.setPosition(&pos);
		*/

		//������������ӽǾ����豸�ӽǾ���
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

		//��δ�������ԭ����The Terrain��
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
	//pos1:�༭�������е�λ��
	HMENU hMenu = GetMenu(hwnd); //�������˵�
	HMENU subMenu1 = GetSubMenu(hMenu,1); //�༭

	HMENU subMenu2 = GetSubMenu(subMenu1,pos1);
	//subMenu2����contain��
	CheckMenuRadioItem(subMenu2,begin,end,dest,MF_BYPOSITION);

}
bool getEditMenuCheckedState(HWND hwnd,int pos1,int dest)
{
	//�ڶ�����������������INDENTIFIER
	MENUITEMINFO info;
	info.cbSize = sizeof(MENUITEMINFO);
	info.fMask = MIIM_STATE;
	//pos1:�༭�������е�λ��
	HMENU hMenu = GetMenu(hwnd); //�������˵�
	HMENU subMenu1 = GetSubMenu(hMenu,1); //�༭

	HMENU subMenu2 = GetSubMenu(subMenu1,pos1);
	//subMenu2����contain��
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
			if(getEditMenuCheckedState(hwnd,1,0) ) //������Ӹ߶ȱ�ѡ����
				if ( triIndex != -1 )
				TheTerrain->changeHeight(triIndex);

			if(getEditMenuCheckedState(hwnd,1,1) ) //ƽ��
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
	case WM_COMMAND: //�˵���Ϣ��Ӧ
		switch(LOWORD(wParam))
		{
			//�ļ�-�˳�
			case ID_40005:
				PostQuitMessage(0);
				break;
			//����
			case ID_40009:
				MessageBox(0,"����","About",0);
				break;

			//˼·����LoadMenu����֮���ò˵������set
			//�༭1-����0
			case ID_40007: //����0-ѡ������1 0
				setEditMenuRadio(hwnd,0,0,2,0);
				break;
			case ID_40008: //����0-ѡ������2 1
				setEditMenuRadio(hwnd,0,0,2,1);
				break;
			case ID_40010: //����0-��ѡ������ 2
				setEditMenuRadio(hwnd,0,0,2,2);
				break;
			case ID_40012: //�߶�1-����0
				setEditMenuRadio(hwnd,1,0,1,0);
				break;
			case ID_40013: //�߶�1-ƽ��1
				setEditMenuRadio(hwnd,1,0,1,1);
				break;
			//�༭1-����2
			case ID_40015: //�༭1-����2-����0,range:0-3
				setEditMenuRadio(hwnd,2,0,3,0);
				break;
			case ID_40016://�༭1-����2-ɾ��1,range:0-3
				setEditMenuRadio(hwnd,2,0,3,1);
				break;
			case ID_40017://�༭1-����2-�ƶ�2,range:0-3
				setEditMenuRadio(hwnd,2,0,3,2);
				break;
			case ID_40018://�༭1-����2-���ı�3,range:0-3
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