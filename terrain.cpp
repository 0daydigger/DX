//////////////////////////////////////////////////////////////////////////////////////////////////
// 
// File: terrain.cpp
// 
// Author: Frank Luna (C) All Rights Reserved
//
// System: AMD Athlon 1800+ XP, 512 DDR, Geforce 3, Windows XP, MSVC++ 7.0 
//
// Desc: Represents a 3D terrain. 
//          
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "terrain.h"
#include <fstream>
#include <cmath>

const DWORD Terrain::TerrainVertex::FVF = D3DFVF_XYZ | D3DFVF_TEX1; //带材质的顶点格式

Terrain::Terrain(IDirect3DDevice9* device,	//设备
				 std::string heightmapFileName, //高度图名称
				 int numVertsPerRow,  //每行点数
				 int numVertsPerCol,  //每列点数
				 int cellSpacing,   //跨度
				 float heightScale) //高度范围 
{
	_device         = device;
	_numVertsPerRow = numVertsPerRow;
	_numVertsPerCol = numVertsPerCol;
	_cellSpacing    = cellSpacing;

	_numCellsPerRow = _numVertsPerRow - 1;
	_numCellsPerCol = _numVertsPerCol - 1;

	_width = _numCellsPerRow * _cellSpacing;
	_depth = _numCellsPerCol * _cellSpacing;

	_numVertices  = _numVertsPerRow * _numVertsPerCol;
	_numTriangles = _numCellsPerRow * _numCellsPerCol * 2;

	_heightScale = heightScale;
	//分配内存，记得释放！
	vInfo = (VerticesInfo*) malloc(sizeof(VerticesInfo) * _numVertices);
	tInfo = (TriangleInfo*) malloc(sizeof(TriangleInfo) * _numTriangles);
	
	memset(vInfo,0,sizeof(VerticesInfo) * _numVertices);
	memset(tInfo,0,sizeof(TriangleInfo) * _numTriangles);

	//vInfo保存的是顶点信息，tInfo保存的是三角形信息，这样tInfo和vInfo就构成了一个三角形反查表。

	// load heightmap
	if( !readRawFile(heightmapFileName) )
	{
		::MessageBox(0, "readRawFile - FAILED", 0, 0);
		::PostQuitMessage(0);
	}

	// scale heights
	for(int i = 0; i < _heightmap.size(); i++)
		_heightmap[i] *= heightScale; 

	// compute the vertices
	if( !computeVertices() )
	{
		::MessageBox(0, "computeVertices - FAILED", 0, 0);
		::PostQuitMessage(0);
	}

	// compute the indices
	if( !computeIndices() )
	{
		::MessageBox(0, "computeIndices - FAILED", 0, 0);
		::PostQuitMessage(0);
	}

	

}

Terrain::~Terrain()
{
	d3d::Release<IDirect3DVertexBuffer9*>(_vb);
	d3d::Release<IDirect3DIndexBuffer9*>(_ib);
	d3d::Release<IDirect3DTexture9*>(_tex);
	free(tInfo);
	free(vInfo);
}

int Terrain::getHeightmapEntry(int row, int col)
{
	return _heightmap[row * _numVertsPerRow + col];
}

void Terrain::setHeightmapEntry(int row, int col, int value)
{
	_heightmap[row * _numVertsPerRow + col] = value;
}
VerticesInfo Terrain::getVerticesInfo(int index)
{
	return vInfo[index];
}
TriangleInfo Terrain::getTriangleInfo(int index)
{
	return tInfo[index];
}
bool Terrain::changeHeight(int triangleIndex)
{
	TriangleInfo triangle = tInfo[triangleIndex];
	//获得了三角形的信息之后，就要修改点的信息了。
	
	TerrainVertex* v = 0;
	_vb->Lock(0, 0, (void**)&v, 0);
	if ( triangle.first >= 0 )
	v[triangle.first]._y += sin(D3DX_PI/2);
	_vb->Unlock();
	return true;
}
int Terrain::pickTriangle(HWND hwnd)
{
	
	D3DXVECTOR3 v;
	D3DXMATRIX matProj;
	POINT pt;
	D3DVIEWPORT9 vp;
	D3DXMATRIX matInverse, matWorld;
	D3DXMATRIX m;
	D3DXMATRIX matView;
	D3DXVECTOR3 rayObjSpace;
	D3DXVECTOR3 rayObjOrigin,rayObjDirection, rayDirection;

	GetCursorPos(&pt);
	ScreenToClient(hwnd, &pt);
	_device->GetTransform(D3DTS_PROJECTION, &matProj);
	_device->GetViewport(&vp);
	_device->GetTransform(D3DTS_WORLD, &matWorld);
	_device->GetTransform(D3DTS_VIEW, &matView);
	
	// Use inverse of matrix
	D3DXVECTOR3 rayPos(pt.x, pt.y,0); // near-plane position
	D3DXVECTOR3 rayDir(pt.x, pt.x,1); // far-plane position
	D3DXVec3Unproject(&rayPos,&rayPos,&vp,&matProj,&matView,&matWorld);
	D3DXVec3Unproject(&rayDir,&rayDir,&vp,&matProj,&matView,&matWorld);
	rayDir -= rayPos; // make a direction from the 2 positions
	D3DXVec3Normalize(&rayDir,&rayDir); // don't know if this is necessary.
	// Transform ray origin and direction by inv matrix
	//
			
	float _u,_v,_w;
	
	for(int i = 0 ; i < _numTriangles; i++)
	{			
		D3DXVECTOR3 p0,p1,p2;
		p0.x = vInfo[tInfo[i].first].x;
		p0.y = vInfo[tInfo[i].first].y;
		p0.z = vInfo[tInfo[i].first].z;

		p1.x = vInfo[tInfo[i].second].x;
		p1.y = vInfo[tInfo[i].second].y;
		p1.z = vInfo[tInfo[i].second].z;

		p2.x = vInfo[tInfo[i].third].x;
		p2.y = vInfo[tInfo[i].third].y;
		p2.z = vInfo[tInfo[i].third].z;

		//初始化工作完成下面判断相交
		if (D3DXIntersectTri(&p0,&p1,&p2,&rayPos,&rayDir,&_u,&_v,&_w) )
		{
			return i;
		}
	} 
	return -1;
}
bool Terrain::computeVertices()
{
	HRESULT hr = 0;

	hr = _device->CreateVertexBuffer(
		_numVertices * sizeof(TerrainVertex),
		D3DUSAGE_WRITEONLY,
		TerrainVertex::FVF,
		D3DPOOL_MANAGED,
		&_vb,
		0);

	if(FAILED(hr))
		return false;

	// coordinates to start generating vertices at
	int startX = -_width / 2;
	int startZ =  _depth / 2;

	// coordinates to end generating vertices at
	int endX =  _width / 2;
	int endZ = -_depth / 2;

	// compute the increment size of the texture coordinates
	// from one vertex to the next.
	float uCoordIncrementSize = 1.0f / (float)_numCellsPerRow;
	float vCoordIncrementSize = 1.0f / (float)_numCellsPerCol;

	TerrainVertex* v = 0;
	_vb->Lock(0, 0, (void**)&v, 0);

	int i = 0;
	for(int z = startZ; z >= endZ; z -= _cellSpacing)
	{
		int j = 0;
		for(int x = startX; x <= endX; x += _cellSpacing)
		{
			// compute the correct index into the vertex buffer and heightmap
			// based on where we are in the nested loop.
			int index = i * _numVertsPerRow + j;

			v[index] = TerrainVertex(
				(float)x,
				(float)_heightmap[index],
				(float)z,
				(float)j * uCoordIncrementSize,
				(float)i * vCoordIncrementSize);
		
			//填充顶点信息
			vInfo[index].x = (float)x;
			vInfo[index].y = (float)_heightmap[index];
			vInfo[index].z = (float)z;

			j++; // next column
		}
		i++; // next row
	}

	_vb->Unlock();

	return true;
}
int Terrain::getTriangleNumber()
{
	return  _numTriangles;
}
bool Terrain::computeIndices()
{
	HRESULT hr = 0;

	hr = _device->CreateIndexBuffer(
		_numTriangles * 3 * sizeof(WORD), // 3 indices per triangle
		D3DUSAGE_WRITEONLY,
		D3DFMT_INDEX16,
		D3DPOOL_MANAGED,
		&_ib,
		0);

	if(FAILED(hr))
		return false;

	WORD* indices = 0;
	_ib->Lock(0, 0, (void**)&indices, 0);

	// index to start of a group of 6 indices that describe the
	// two triangles that make up a quad
	int baseIndex = 0;
	int tInfoIndex = 0;
	// loop through and compute the triangles of each quad
	for(int i = 0; i < _numCellsPerCol; i++)
	{
		for(int j = 0; j < _numCellsPerRow; j++)
		{
			indices[baseIndex]     =   i   * _numVertsPerRow + j;
			indices[baseIndex + 1] =   i   * _numVertsPerRow + j + 1;
			indices[baseIndex + 2] =  (i+1) * _numVertsPerRow + j;

			tInfo[tInfoIndex].first =  i   * _numVertsPerRow + j;
			tInfo[tInfoIndex].second = i   * _numVertsPerRow + j + 1;
			tInfo[tInfoIndex].third = (i+1) * _numVertsPerRow + j;
			tInfoIndex++;

			indices[baseIndex + 3] = (i+1) * _numVertsPerRow + j;
			indices[baseIndex + 4] =   i   * _numVertsPerRow + j + 1;
			indices[baseIndex + 5] = (i+1) * _numVertsPerRow + j + 1;

			tInfo[tInfoIndex].first = (i+1) * _numVertsPerRow + j;
			tInfo[tInfoIndex].second =  i   * _numVertsPerRow + j + 1;
			tInfo[tInfoIndex].third = (i+1) * _numVertsPerRow + j + 1;
			tInfoIndex++;

			// next quad
			baseIndex += 6;
		}
	}

	_ib->Unlock();

	return true;
}

bool Terrain::loadTexture(std::string fileName)
{
	HRESULT hr = 0;

	hr = D3DXCreateTextureFromFile(
		_device,
		fileName.c_str(),
		&_tex);

	if(FAILED(hr))
		return false;

	return true;
}

bool Terrain::genTexture(D3DXVECTOR3* directionToLight)
{
	// Method fills the top surface of a texture procedurally.  Then
	// lights the top surface.  Finally, it fills the other mipmap
	// surfaces based on the top surface data using D3DXFilterTexture.

	HRESULT hr = 0;

	// texel for each quad cell
	int texWidth  = _numCellsPerRow;
	int texHeight = _numCellsPerCol;

	// create an empty texture
	hr = D3DXCreateTexture(
		_device,
		texWidth, texHeight,
		0, // create a complete mipmap chain
		0, // usage
		D3DFMT_X8R8G8B8,// 32 bit XRGB format
		D3DPOOL_MANAGED, &_tex);

	if(FAILED(hr))
		return false;

	D3DSURFACE_DESC textureDesc; 
	_tex->GetLevelDesc(0 /*level*/, &textureDesc);

	// make sure we got the requested format because our code 
	// that fills the texture is hard coded to a 32 bit pixel depth.
	if( textureDesc.Format != D3DFMT_X8R8G8B8 )
		return false;
		
	D3DLOCKED_RECT lockedRect;
	_tex->LockRect(0/*lock top surface*/, &lockedRect, 
		0 /* lock entire tex*/, 0/*flags*/);         

	DWORD* imageData = (DWORD*)lockedRect.pBits;
	for(int i = 0; i < texHeight; i++)
	{
		for(int j = 0; j < texWidth; j++)
		{
			D3DXCOLOR c;

			// get height of upper left vertex of quad.
			float height = (float)getHeightmapEntry(i, j) / _heightScale;

			if( (height) < 42.5f ) 		 c = d3d::BEACH_SAND;
			else if( (height) < 85.0f )	 c = d3d::LIGHT_YELLOW_GREEN;
			else if( (height) < 127.5f ) c = d3d::PUREGREEN;
			else if( (height) < 170.0f ) c = d3d::DARK_YELLOW_GREEN;
			else if( (height) < 212.5f ) c = d3d::DARKBROWN;
			else	                     c = d3d::WHITE;

			// fill locked data, note we divide the pitch by four because the
			// pitch is given in bytes and there are 4 bytes per DWORD.
			imageData[i * lockedRect.Pitch / 4 + j] = (D3DCOLOR)c;
		}
	}

	_tex->UnlockRect(0);

	if(!lightTerrain(directionToLight))
	{
		::MessageBox(0, "lightTerrain() - FAILED", 0, 0);
		return false;
	}
	
	hr = D3DXFilterTexture(
		_tex,
		0, // default palette
		0, // use top level as source level
		D3DX_DEFAULT); // default filter

	if(FAILED(hr))
	{
		::MessageBox(0, "D3DXFilterTexture() - FAILED", 0, 0);
		return false;
	}

	return true;
}

bool Terrain::lightTerrain(D3DXVECTOR3* directionToLight)
{
	HRESULT hr = 0;

	D3DSURFACE_DESC textureDesc; 
	_tex->GetLevelDesc(0 /*level*/, &textureDesc);

	// make sure we got the requested format because our code that fills the
	// texture is hard coded to a 32 bit pixel depth.
	if( textureDesc.Format != D3DFMT_X8R8G8B8 )
		return false;
		
	D3DLOCKED_RECT lockedRect;
	_tex->LockRect(
		0,          // lock top surface level in mipmap chain
		&lockedRect,// pointer to receive locked data
		0,          // lock entire texture image
		0);         // no lock flags specified

	DWORD* imageData = (DWORD*)lockedRect.pBits;
	for(int i = 0; i < textureDesc.Height; i++)
	{
		for(int j = 0; j < textureDesc.Width; j++)
		{
			// index into texture, note we use the pitch and divide by 
			// four since the pitch is given in bytes and there are 
			// 4 bytes per DWORD.
			int index = i * lockedRect.Pitch / 4 + j;

			// get current color of quad
			D3DXCOLOR c( imageData[index] );

			// shade current quad
			c *= computeShade(i, j, directionToLight);;

			// save shaded color
			imageData[index] = (D3DCOLOR)c;
		}
	}

	_tex->UnlockRect(0);

	return true;
}

float Terrain::computeShade(int cellRow, int cellCol, D3DXVECTOR3* directionToLight)
{
	// get heights of three vertices on the quad
	float heightA = getHeightmapEntry(cellRow,   cellCol);
	float heightB = getHeightmapEntry(cellRow,   cellCol+1);
	float heightC = getHeightmapEntry(cellRow+1, cellCol);

	// build two vectors on the quad
	D3DXVECTOR3 u(_cellSpacing, heightB - heightA, 0.0f);
	D3DXVECTOR3 v(0.0f,         heightC - heightA, -_cellSpacing);

	// find the normal by taking the cross product of two
	// vectors on the quad.
	D3DXVECTOR3 n;
	D3DXVec3Cross(&n, &u, &v);
	D3DXVec3Normalize(&n, &n);

	float cosine = D3DXVec3Dot(&n, directionToLight);

	if(cosine < 0.0f)
		cosine = 0.0f;

	return cosine;
}

bool Terrain::readRawFile(std::string fileName)
{
	// Restriction: RAW file dimensions must be >= to the
	// dimensions of the terrain.  That is a 128x128 RAW file
	// can only be used with a terrain constructed with at most
	// 128x128 vertices.

	// A height for each vertex
	std::vector<BYTE> in( _numVertices );

	std::ifstream inFile(fileName.c_str(), std::ios_base::binary);

	if( inFile == 0 )
		return false;

	inFile.read(
		(char*)&in[0], // buffer
		in.size());// number of bytes to read into buffer

	inFile.close();

	// copy BYTE vector to int vector
	_heightmap.resize( _numVertices );

	for(int i = 0; i < in.size(); i++)
		_heightmap[i] = in[i];

	return true;
}

float Terrain::getHeight(float x, float z)
{
	// Translate on xz-plane by the transformation that takes
	// the terrain START point to the origin.
	x = ((float)_width / 2.0f) + x;
	z = ((float)_depth / 2.0f) - z;

	// Scale down by the transformation that makes the 
	// cellspacing equal to one.  This is given by 
	// 1 / cellspacing since; cellspacing * 1 / cellspacing = 1.
	x /= (float)_cellSpacing;
	z /= (float)_cellSpacing;

	// From now on, we will interpret our positive z-axis as
	// going in the 'down' direction, rather than the 'up' direction.
	// This allows to extract the row and column simply by 'flooring'
	// x and z:

	float col = ::floorf(x);
	float row = ::floorf(z);

	// get the heights of the quad we're in:
	// 
    //  A   B
    //  *---*
    //  | / |
    //  *---*  
    //  C   D

	float A = getHeightmapEntry(row,   col);
	float B = getHeightmapEntry(row,   col+1);
	float C = getHeightmapEntry(row+1, col);
	float D = getHeightmapEntry(row+1, col+1);

	//
	// Find the triangle we are in:
	//

	// Translate by the transformation that takes the upper-left
	// corner of the cell we are in to the origin.  Recall that our 
	// cellspacing was nomalized to 1.  Thus we have a unit square
	// at the origin of our +x -> 'right' and +z -> 'down' system.
	float dx = x - col;
	float dz = z - row;

	// Note the below compuations of u and v are unneccessary, we really
	// only need the height, but we compute the entire vector to emphasis
	// the books discussion.
	float height = 0.0f;
	if(dz < 1.0f - dx)  // upper triangle ABC
	{
		float uy = B - A; // A->B
		float vy = C - A; // A->C

		// Linearly interpolate on each vector.  The height is the vertex
		// height the vectors u and v originate from {A}, plus the heights
		// found by interpolating on each vector u and v.
		height = A + d3d::Lerp(0.0f, uy, dx) + d3d::Lerp(0.0f, vy, dz);
	}
	else // lower triangle DCB
	{
		float uy = C - D; // D->C
		float vy = B - D; // D->B

		// Linearly interpolate on each vector.  The height is the vertex
		// height the vectors u and v originate from {D}, plus the heights
		// found by interpolating on each vector u and v.
		height = D + d3d::Lerp(0.0f, uy, 1.0f - dx) + d3d::Lerp(0.0f, vy, 1.0f - dz);
	}

	return height;
}

bool Terrain::draw(D3DXMATRIX* world, bool drawTris)
{
	HRESULT hr = 0;

	if( _device )
	{
		_device->SetTransform(D3DTS_WORLD, world);

		_device->SetStreamSource(0, _vb, 0, sizeof(TerrainVertex));
		_device->SetFVF(TerrainVertex::FVF);
		_device->SetIndices(_ib);
		
		/*	这里是设置纹理用的	*/
	//	_device->SetTexture(0, _tex);

		// turn off lighting since we're lighting it ourselves
		_device->SetRenderState(D3DRS_LIGHTING, false);

		hr =_device->DrawIndexedPrimitive(
			D3DPT_TRIANGLELIST,
			0,
			0,
			_numVertices,
			0,
			_numTriangles);

		_device->SetRenderState(D3DRS_LIGHTING, true);

		if( drawTris )
		{
			_device->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
			hr =_device->DrawIndexedPrimitive(
				D3DPT_TRIANGLELIST,
				0,
				0,
				_numVertices,
				0,
				_numTriangles);

			_device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
		}

		if(FAILED(hr))
			return false;
	}

	return true;
}

