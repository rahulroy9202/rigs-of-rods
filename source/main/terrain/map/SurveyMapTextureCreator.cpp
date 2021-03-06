/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "SurveyMapTextureCreator.h"

#include "BeamFactory.h"
#include "IWater.h"
#include "ResourceBuffer.h"
#include "SurveyMapManager.h"
#include "TerrainManager.h"

using namespace Ogre;

int SurveyMapTextureCreator::mCounter = 0;

SurveyMapTextureCreator::SurveyMapTextureCreator() :
	  mCamera(NULL)
	, mRttTex(NULL)
	, mStatics(NULL)
	, mTextureUnitState(NULL)
	, mViewport(NULL)
	, mMapCenter(Vector2::ZERO)
	, mMapSize(Vector3::ZERO)
	, mMapZoom(0.0f)
{
	mCounter++;
	init();
}

bool SurveyMapTextureCreator::init()
{
	TexturePtr texture = TextureManager::getSingleton().createManual(getTextureName(), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_2D, 2048, 2048, TU_RENDERTARGET, PF_R8G8B8, TU_RENDERTARGET, new ResourceBuffer());
	
	if ( texture.isNull() ) return false;;

	mRttTex = texture->getBuffer()->getRenderTarget();

	if ( !mRttTex ) return false;

	mRttTex->setAutoUpdated(false);

	mCamera = gEnv->sceneManager->createCamera(getCameraName());

	mViewport = mRttTex->addViewport(mCamera);
	mViewport->setBackgroundColour(ColourValue::Black);
	mViewport->setOverlaysEnabled(false);
	mViewport->setShadowsEnabled(false);
	mViewport->setSkiesEnabled(false);

	mMaterial = MaterialManager::getSingleton().create(getMaterialName(), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

	if ( mMaterial.isNull() ) return false;

	mTextureUnitState = mMaterial->getTechnique(0)->getPass(0)->createTextureUnitState(getTextureName());

	mRttTex->addListener(this);

	mCamera->setFixedYawAxis(false);
	mCamera->setProjectionType(PT_ORTHOGRAPHIC);
	mCamera->setNearClipDistance(1.0f);

	return true;
}

void SurveyMapTextureCreator::setStaticGeometry(StaticGeometry *staticGeometry)
{
	mStatics = staticGeometry;
}

void SurveyMapTextureCreator::update()
{
	if ( !mRttTex ) return;

	mMapSize = Vector3::ZERO;
	mMapCenter = Vector2::ZERO;
	mMapZoom = 0.0f;

	if (gEnv->terrainManager)
		mMapSize = gEnv->surveyMap->getMapSize();

	if (gEnv->surveyMap)
	{
		mMapCenter = gEnv->surveyMap->getMapCenter();
		mMapZoom   = gEnv->surveyMap->getMapZoom();
	}

	float orthoWindowWidth  = mMapSize.x - (mMapSize.x - 20.0f) * mMapZoom;
	float orthoWindowHeight = mMapSize.z - (mMapSize.z - 20.0f) * mMapZoom;

	mCamera->setFarClipDistance(mMapSize.y + 3.0f);
	mCamera->setOrthoWindow(orthoWindowWidth, orthoWindowHeight);
	mCamera->setPosition(Vector3(mMapCenter.x, mMapSize.y + 2.0f, mMapCenter.y));
	mCamera->lookAt(Vector3(mMapCenter.x, 0.0f, mMapCenter.y));

	preRenderTargetUpdate();

	mRttTex->update();

	postRenderTargetUpdate();
}

String SurveyMapTextureCreator::getMaterialName()
{
	return "MapRttMat" + TOSTRING(mCounter);
}

String SurveyMapTextureCreator::getCameraName()
{
	return "MapRttCam" + TOSTRING(mCounter);
}

String SurveyMapTextureCreator::getTextureName()
{
	return "MapRttTex" + TOSTRING(mCounter);
}

void SurveyMapTextureCreator::preRenderTargetUpdate()
{
	Beam **trucks = BeamFactory::getSingleton().getTrucks();

	float f = 20.0f + 30.0f * mMapZoom;

	for (int i=0; i < BeamFactory::getSingleton().getTruckCount(); i++)
		if (trucks[i])
			trucks[i]->preMapLabelRenderUpdate(true, f);

	if (mStatics)
		mStatics->setRenderingDistance(0);

	IWater* water = gEnv->terrainManager->getWater();
	if (water)
	{
		water->setCamera(mCamera);
		water->moveTo(water->getHeight());
		water->update();
	}
}

void SurveyMapTextureCreator::postRenderTargetUpdate()
{
	Beam **trucks = BeamFactory::getSingleton().getTrucks();

	for (int i=0; i < BeamFactory::getSingleton().getTruckCount(); i++)
		if (trucks[i])
			trucks[i]->preMapLabelRenderUpdate(false);

	if (mStatics)
		mStatics->setRenderingDistance(1000);

	IWater* water = gEnv->terrainManager->getWater();
	if (water)
	{
		water->setCamera(gEnv->mainCamera);
		water->moveTo(water->getHeight());
		water->update();
	}
}
