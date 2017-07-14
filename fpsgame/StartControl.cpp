#include "StarControl.h"
#include "ObjectParticles.h"
#include "Engine.h"
#include "Game.h"
#include "Object.h"
#include "Player.h"
#include "Common.h"
#include "App.h"

CStarControl::CStarControl(void)
{
	m_pStarNormal = NULL;
	m_pStarClick = NULL;
	m_fViewDistance = 0.5f;
	m_nClickState = 0;
	m_fClickScale = 1.0f;
	Init();
}

CStarControl::~CStarControl(void)
{
	g_Engine.pGame->RemoveNode(m_pStarNormal);
	g_Engine.pGame->RemoveNode(m_pStarClick);
}

int CStarControl::Init()
{
	return Load("data/StarControl/Star_mesh.node", "data/StarControl/Star_click.node");
}

void CStarControl::Update(const vec3& vPos, const vec3& vDir)
{
	m_fClickScale += m_nClickState * g_Engine.pGame->GetIFps() * 5.5f;
	if (m_fClickScale <= 0.8f)
	{
		m_nClickState = 1;
	}
	else if (m_fClickScale >= 1.0f)
	{
		m_fClickScale = 1.0f;
		m_nClickState = 0;
	}
	float fViewDistance = m_fViewDistance;
	mat4 matStar = Translate(vPos + vDir * fViewDistance);
	CPlayer* pPlayer = g_Engine.pGame->GetPlayer();
	if (pPlayer)
	{
		vec3 x = pPlayer->GetModelview().getRow3(0);
		vec3 y = pPlayer->GetModelview().getRow3(1);
		vec3 z = pPlayer->GetModelview().getRow3(2);

		matStar.setColumn3(0, x);
		matStar.setColumn3(1, -z);
		matStar.setColumn3(2, y);
	}
	vec4 pos = vec4(1.0f, 0.0f, 0.0f, 1.0f);
	float s = CCommon::CalcAxisScale(g_Engine.pGame->GetPlayer()->GetModelview(), g_Engine.pGame->GetPlayer()->GetFov(),
		vec4(matStar.getColumn3(3), 1.0f),
		200.0f,
		CMathCore::Itof(g_Engine.pApp->GetHeight()));

	m_pStarNormal->SetWorldTransform(matStar * Scale(s, 1.0f, s));
	m_pStarClick->SetWorldTransform(matStar * Scale(m_fClickScale, 1.0f, m_fClickScale) * Scale(s, 1.0f, s));
}
void CStarControl::Click()
{
	m_nClickState = -1;
}
int CStarControl::Load(const char* strNormal, const char* strClick)
{
	if (m_pStarClick && m_pStarNormal)
	{
		g_Engine.pGame->RemoveNode(m_pStarNormal);
		g_Engine.pGame->RemoveNode(m_pStarClick);
	}

	m_pStarNormal = (CBRObject*)g_Engine.pGame->LoadNode(strNormal);
	m_pStarClick = (CBRObject*)g_Engine.pGame->LoadNode(strClick);

	if (m_pStarClick && m_pStarNormal)
	{
		return 1;
	}

	return 0;
}

int CStarControl::isEnable()
{
	return m_pStarClick->IsEnabled();
}
void CStarControl::SetEnable(int nEnable)
{
	m_pStarClick->SetEnabled(nEnable);
	m_pStarNormal->SetEnabled(nEnable);
}
void CStarControl::SetColor(const vec4& vColor)
{
	m_pStarNormal->SetObjectColor(vColor);
}