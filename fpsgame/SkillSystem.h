#pragma once
#include "MessageBase.h"

class CInterfaceBase;
class CGameProcess;
class CSkillSystem : public CMessageBase		//����ϵͳ���̳�����Ϣ����
{
public:
	CSkillSystem(CGameProcess*	pGameProcess);
	virtual ~CSkillSystem(void);

public:
	void		Update(float ifps);
	void		Reset();
protected:
	int	OnParticleHit(void* v1, void* v2, void* v3);
	int	OnSkillCallback(void* pVoid);
	CGameProcess*	   m_pGameProcess;
};