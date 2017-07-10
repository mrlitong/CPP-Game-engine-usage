#include "Engine.h"
#include "Game.h"
#include "ObjectDummy.h"
#include "BodyRigid.h"
#include "BodyDummy.h"
#include "Physics.h"
#include "ShapeCapsule.h"
#include "ActorBase.h"
#include "Visualizer.h"
#include "sys/SysControl.h"


using namespace MathLib;

#ifdef MEMORY_INFO
#define new new(__FILE__, __LINE__) 

#endif // MEMORY_INFO

/*
Test high performance.
*/
#define ACTOR_BASE_IFPS             (1.0f / 120.0f)
#define ACTOR_BASE_CLAMP            15.0f
#define ACTOR_BASE_COLLISIONS       4


/*
*/
CActorBase::CActorBase()
{
	m_vUp = vec3(0.0f, 0.0f, 1.0f);
	m_pObject = new CObjectDummy();
	m_pDummy = new CBodyDummy();
	m_pShape = new CShapeCapsule(1.0f, 1.0f);

	m_nFlush = 0;
	m_vPosition = Vec3_zero;
	m_vVelocity = Vec3_zero;
	m_fPhiAngle = 0.0f;				//倾斜角		二维平面坐标系中，直线向Y周延伸的方向与X轴正向之间的夹角
	m_fThetaAngle = 0.0f;			//方位角，正北方那条线与当前线条按照顺时针走过的角度


	for (int i = 0; i < NUM_STATES; i++)
	{
		m_pStates[i] = 0;
		m_pTimes[i] = 0.0f;
	}
	m_pDummy->SetEnabled(1);
	m_pObject->SetBody(NULL);
	m_pObject->SetBody(m_pDummy);
	m_pShape->SetBody(NULL);
	m_pShape->SetBody(m_pDummy);

	m_pObject->SetWorldTransform(Get_Body_Transform());
	m_pShape->SetRestitution(0.0f);
	m_pShape->SetCollisionMask(2);


	SetEnabled(1);
	SetViewDirection(vec3(0.0f, 1.0f, 0.0f));

	SetCollision(1);
	SetCollisionRadius(0.3f);
	SetCollisionHeight(1.0f);

	

	SetMinVelocity(2.0f);
	SetMaxVelocity(4.0f);

	SetAcceleration(8.0f);
	SetDamping(8.0f);
	SetJumping(1.5f);

	SetFriction(2.0f);
	SetGround(0);
	SetCeiling(0);
}

CActorBase::~CActorBase()
{
	m_pDummy->SetObject(NULL);
	delete m_pObject;
	delete m_pDummy;
}

void CActorBase::SetEnabled(int enable)
{
	m_nEnable = enable;
	m_pDummy->SetEnabled(m_nEnable);
}


int CActorBase::IsEnabled() const
{
	return m_nEnable;
}
void CActorBase::Update(float ifps)
{
	if (!m_nEnable)
	{
		return;
	}

	// impulse
	vec3 impulse = vec3_zero;

	// ortho basis
	vec3 tangent, binormal;
	OrthoBasis(m_vUp, tangent, binormal);

	// current basis
	vec3 x = quat(m_vUp, -m_fPhiAngle) * binormal;
	vec3 y = Normalize(Cross(m_vUp, x));
	vec3 z = Normalize(Cross(x, y));

	handle states
		Update_States(1, ifps);

	// old velocity
	float x_velocity = Dot(x, m_vVelocity);
	float y_velocity = Dot(y, m_vVelocity);
	float z_velocity = Dot(z, m_vVelocity);
	// movement
	if (m_pStates[STATE_FORWARD]) impulse += x;
	if (m_pStates[STATE_BACKWARD]) impulse -= x;
	if (m_pStates[STATE_MOVE_LEFT]) impulse += y;
	if (m_pStates[STATE_MOVE_RIGHT]) impulse -= y;
	impulse.normalize();
	//velocity
	if (m_pStates[STATE_RUN])
		impulse *= m_fMaxVelocity;
	else
		impulse *= m_fMinVelocity;
	// jump
	if (m_pStates[STATE_JUMP] == STATE_BEGIN)
	{
		impulse += z * CMathCore::Sqrt(2.0f * 9.8f * m_fJumping) / (m_fAcceleration * ifps);
	}

	// rotate velocity
	if (GetGround())
	{
		m_vVelocity = x * x_velocity + y * y_velocity + z * z_velocity;
	}

	// time
	float time = ifps * g_Engine.pPhysics->GetScale();

	// target velocity
	float target_velocity = Length(vec2(Dot(x, impulse), Dot(y, impulse)));

	// penetration tolerance
	float penetration = g_Engine.pPhysics->GetPenetrationTolerance();
	float penetration_2 = penetration * 2.0f;

	// frozen linear velocity
	float frozen_velocity = g_Engine.pPhysics->GetFrozenLinearVelocity();

	// friction
	float friction = 0.0f;
	if (target_velocity < EPSILON)
	{
		friction = m_fFriction;
	}

	//clear collision flags
	if (GetCollision())
	{
		m_nGround = 0;
		m_nCeiling = 0;
	}
	// movement
	do
	{
		// adaptive time step
		float ifps = Min(time, ACTOR_BASE_IFPS);
		time -= ifps;

		// save old velocity
		float old_velocity = Length(vec2(Dot(x, m_vVelocity), Dot(y, m_vVelocity)));

		// integrate velocity
		m_vVelocity += impulse * (m_fAcceleration * ifps);
		m_vVelocity += g_Engine.pPhysics->GetGravity() * ifps;

		// damping
		float current_velocity = Length(vec2(Dot(x, m_vVelocity), Dot(y, m_vVelocity)));
		if (target_velocity < EPSILON || current_velocity > target_velocity)
		{
			m_vVelocity = (x * Dot(x, m_vVelocity) + y * Dot(y, m_vVelocity)) * CMathCore::Exp(-m_fDamping * ifps) + z * Dot(z, m_vVelocity);
		}

		// clamp maximum velocity
		current_velocity = Length(vec2(Dot(x, m_vVelocity), Dot(y, m_vVelocity)));
		if (current_velocity > old_velocity)
		{
			if (current_velocity > target_velocity)
			{
				m_vVelocity = (x * Dot(x, m_vVelocity) + y * Dot(y, m_vVelocity)) * target_velocity / current_velocity + z * Dot(z, m_vVelocity);
			}
		}

		// frozen velocity
		int is_frozen = 0;
		if (current_velocity < frozen_velocity)
		{
			m_vVelocity = z * Dot(z, m_vVelocity);
			is_frozen = 1;
		}

		// integrate position
		//m_vPosition += Vec3(m_vVelocity * ifps);

		// world collision
		if (GetCollision())
		{
			// get collision
			vec3 tangent, binormal;
			const Vec3 *caps = m_pShape->GetCaps();
			for (int i = 0; i < ACTOR_BASE_COLLISIONS; i++)
			{
				m_pDummy->SetTransform(Get_Body_Transform());
				m_pShape->GetCollision(m_vecContacts, 0.0f);
				if (m_vecContacts.Size() == 0) break;
				float inum_contacts = 1.0f / CMathCore::Itof(m_vecContacts.Size());
				for (int j = 0; j < m_vecContacts.Size(); j++)
				{
					const CShape::Contact &c = m_vecContacts[j];

					vec3 normalCollision = c.normal;

					if (is_frozen && c.depth < penetration_2)
					{
						m_vPosition += Vec3(z * (Max(c.depth - penetration, 0.0f) * inum_contacts * Dot(z, normalCollision)));
					}
					else
					{
						m_vPosition += Vec3(normalCollision * (Max(c.depth - penetration, 0.0f) * inum_contacts));
						is_frozen = 0;
					}
					float normal_velocity = Dot(normalCollision, m_vVelocity);
					if (normal_velocity < 0.0f)
					{
						m_vVelocity -= normalCollision * normal_velocity;
					}
					if (friction > EPSILON)
					{
						OrthoBasis(c.normal, tangent, binormal);
						float tangent_velocity = Dot(tangent, m_vVelocity);
						float binormal_velocity = Dot(binormal, m_vVelocity);
						if (CMathCore::Abs(tangent_velocity) > EPSILON || CMathCore::Abs(binormal_velocity) > EPSILON) {
							float friction_velocity = Clamp(Max(-normal_velocity, 0.0f) * friction * CMathCore::RSqrt(tangent_velocity * tangent_velocity + binormal_velocity * binormal_velocity), -1.0f, 1.0f);
							m_vVelocity -= tangent * tangent_velocity * friction_velocity;
							m_vVelocity -= binormal * binormal_velocity * friction_velocity;
						}
					}
					if (Dot(c.normal, m_vUp) > 0.5f && Dot(vec3(c.point - caps[0]), m_vUp) < 0.0f) m_nGround = 1;
					if (Dot(c.normal, m_vUp) < -0.5f && Dot(vec3(c.point - caps[1]), m_vUp) > 0.0f) m_nCeiling = 1;
				}
			}


			m_vPosition += Vec3(m_vVelocity * ifps);
		}
		while (time > EPSILON);

		// current position
		m_pObject->SetWorldTransform(Get_Body_Transform());
		m_WorldBoundBox.Set(m_BoundBox, Translate(m_vPosition));
		m_WorldBoundSphere.Set(m_BoundSphere, Translate(m_vPosition));
	}
}

/*
*/
void CActorBase::Update_Bounds()
{
	float radius = m_pShape->GetRadius();
	float hheight = m_pShape->GetHHeight();
	m_BoundBox.Set(vec3(-radius, -radius, 0.0f), vec3(radius, radius, (radius + hheight) * 2.0f));
	m_BoundSphere.Set(vec3(0.0f, 0.0f, radius + hheight), radius + hheight);

	m_WorldBoundBox.Set(m_BoundBox, Translate(m_vPosition));
	m_WorldBoundSphere.Set(m_BoundSphere, Translate(m_vPosition));
}

/*
*/
void CActorBase::SetIntersectionMask(int mask)
{
	m_pShape->SetIntersectionMask(mask);
}

int CActorBase::GetIntersectionMask() const
{
	return m_pShape->GetIntersectionMask();
}

/*
*/
void CActorBase::SetCollision(int c)
{
	m_nCollision = c;
}

int CActorBase::GetCollision() const
{
	return m_nCollision;
}
void CActorBase::SetCollisionMask(int mask)
{
	m_pShape->SetCollisionMask(mask);
}

int CActorBase::GetCollisionMask() const
{
	return m_pShape->GetCollisionMask();
}
void CActorBase::SetCollisionRadius(float radius)
{
	if (!Compare(m_pShape->GetRadius(), radius))
	{
		m_pDummy->SetPreserveTransform(Mat4(Translate(m_vUp * (radius - m_pShape->GetRadius()))) * m_pDummy->GetTransform());
		m_pShape->SetRadius(radius);
	}
	Update_Bounds();
}

float CActorBase::GetCollisionRadius() const
{
	return m_pShape->GetRadius();
}

/*
*/
void CActorBase::SetCollisionHeight(float height)
{
	if (!Compare(m_pShape->GetHeight(), height))
	{
		m_pDummy->SetPreserveTransform(Mat4(Translate(m_vUp * (height - m_pShape->GetHeight()) * 0.5f)) * m_pDummy->GetTransform());
		m_pShape->SetHeight(height);
	}
	Update_Bounds();
}
float CActorBase::GetCollisionHeight() const
{
	return m_pShape->GetHeight();
}

void CActorBase::SetMinVelocity(float velocity)
{
	m_fMinVelocity = Max(velocity, 0.0f);
}
float CActorBase::GetMinVelocity() const
{
	return m_fMinVelocity;
}

/*
*/
void CActorBase::SetMaxVelocity(float velocity)
{
	m_fMaxVelocity = Max(velocity, 0.0f);
}

float CActorBase::GetMaxVelocity() const
{
	return m_fMaxVelocity;
}

/*
*/
void CActorBase::SetAcceleration(float accel)
{
	m_fAcceleration = Max(accel, 0.0f);
}

float CActorBase::GetAcceleration() const
{
	return m_fAcceleration;
}

/*
*/
void CActorBase::SetDamping(float d)
{
	m_fDamping = Max(d, 0.0f);
}

float CActorBase::GetDamping() const
{
	return m_fDamping;
}

/*
*/
void CActorBase::SetJumping(float j)
{
	m_fJumping = Max(j, 0.0f);
}
float CActorBase::GetJumping() const
{
	return m_fJumping;
}

/*
*/
void CActorBase::SetViewDirection(const vec3 &d)
{
	m_vDirection = Normalize(d);

	// ortho basis
	vec3 tangent, binormal;
	OrthoBasis(m_vUp, tangent, binormal);

	// decompose direction
	m_fPhiAngle = CMathCore::ATan2(Dot(m_vDirection, tangent), Dot(m_vDirection, binormal)) * RAD2DEG;
	m_fThetaAngle = CMathCore::ACos(Clamp(Dot(m_vDirection, m_vUp), -1.0f, 1.0f)) * RAD2DEG - 90.0f;

	m_pObject->SetWorldTransform(Get_Body_Transform());
}

const vec3 &CActorBase::GetViewDirection() const
{
	return m_vDirection;
}
/******************************************************************************\
*
* States
*
\******************************************************************************/

/*
*/
int CActorBase::GetState(int state) const
{
	assert(state >= 0 && state < NUM_STATES && "CPlayerActor::GetState(): bad state number");
	return m_pStates[state];
}

float CActorBase::GetStateTime(int state) const
{
	assert(state >= 0 && state < NUM_STATES && "CPlayerActor::GetStateTime(): bad state number");
	return m_pTimes[state];
}

/******************************************************************************\
*
* Contacts
*
\******************************************************************************/

/*
*/
int CActorBase::GetNumContacts() const
{
	return m_vecContacts.Size();
}

const CShape::Contact &CActorBase::GetContact(int num) const
{
	return m_vecContacts[num];
}

/*
*/
void CActorBase::SetGround(int g)
{
	m_nGround = g;
}

int CActorBase::GetGround() const
{
	return m_nGround;
}

/*
*/
void CActorBase::SetCeiling(int c)
{
	m_nCeiling = c;
}

int CActorBase::GetCeiling() const
{
	return m_nCeiling;
}

/*
*/
Mat4 CActorBase::Get_Body_Transform() const
{
	Vec3 center = m_vPosition + Vec3(m_vUp * (m_pShape->GetHHeight() + m_pShape->GetRadius()));
	return SetTo(center, center + Vec3(m_vDirection - m_vUp * Dot(m_vDirection, m_vUp)), m_vUp) * Mat4(RotateX(-90.0f) * RotateZ(90.0f));
}

/*
*/
int CActorBase::Update_State(int condition, int state, int begin, int end, float ifps)
{
	// disabled to begin
	if (condition && m_pStates[state] == STATE_DISABLED && begin)
	{
		m_pStates[state] = STATE_BEGIN;
		m_pTimes[state] = 0.0f;
		return STATE_BEGIN;
	}

	// enabled or begin to end
	if (condition == 0 && (m_pStates[state] == STATE_ENABLED || m_pStates[state] == STATE_BEGIN) && end)
	{
		m_pStates[state] = STATE_END;
		return STATE_END;
	}
	// begin to enabled
	if ((condition && m_pStates[state] == STATE_BEGIN) || m_pStates[state] == STATE_ENABLED)
	{
		m_pStates[state] = STATE_ENABLED;
		m_pTimes[state] += ifps;
		return STATE_ENABLED;
	}

	// end to disabled
	if (m_pStates[state] == STATE_END)
	{
		m_pStates[state] = STATE_DISABLED;
		return STATE_DISABLED;
	}

	return STATE_DISABLED;

}

void CActorBase::Update_States(int enabled, float ifps)
{
	// handle states
	if (enabled)
	{
		if (g_pSysControl->GetState(CSysControl::STATE_FORWARD) && g_pSysControl->GetState(CSysControl::STATE_BACKWARD))
		{
			Update_State(0, STATE_FORWARD, 1, 1, ifps);
			Update_State(0, STATE_BACKWARD, 1, 1, ifps);
		}
		else
		{
			Update_State(g_pSysControl->GetState(CSysControl::STATE_FORWARD), STATE_FORWARD, 1, 1, ifps);
			Update_State(g_pSysControl->GetState(CSysControl::STATE_BACKWARD), STATE_BACKWARD, 1, 1, ifps);
		}
		if (g_pSysControl->GetState(CSysControl::STATE_MOVE_LEFT) && g_pSysControl->GetState(CSysControl::STATE_MOVE_RIGHT))
		{
			Update_State(0, STATE_MOVE_LEFT, 1, 1, ifps);
			Update_State(0, STATE_MOVE_RIGHT, 1, 1, ifps);
		}
		else
		{
			Update_State(g_pSysControl->GetState(CSysControl::STATE_MOVE_LEFT), STATE_MOVE_LEFT, 1, 1, ifps);
			Update_State(g_pSysControl->GetState(CSysControl::STATE_MOVE_RIGHT), STATE_MOVE_RIGHT, 1, 1, ifps);
		}

		Update_State(g_pSysControl->GetState(CSysControl::STATE_CROUCH), STATE_CROUCH, 1, 1, ifps);
		Update_State(g_pSysControl->GetState(CSysControl::STATE_JUMP), STATE_JUMP, m_nGround, 1, ifps);
		Update_State(g_pSysControl->GetState(CSysControl::STATE_RUN), STATE_RUN, 1, 1, ifps);
	}
	// disable states
	else
	{
		Update_State(0, STATE_FORWARD, 1, 1, ifps);
		Update_State(0, STATE_BACKWARD, 1, 1, ifps);
		Update_State(0, STATE_MOVE_LEFT, 1, 1, ifps);
		Update_State(0, STATE_MOVE_RIGHT, 1, 1, ifps);
		Update_State(0, STATE_CROUCH, 1, 1, ifps);
		Update_State(0, STATE_JUMP, m_nGround, m_nGround, ifps);
		Update_State(0, STATE_RUN, 1, 1, ifps);
	}
}


/******************************************************************************\
*
* Bounds
*
\******************************************************************************/

/*
*/
const CBoundBox &CActorBase::GetBoundBox() const
{
	return m_BoundBox;
}

const CBoundSphere &CActorBase::GetBoundSphere() const
{
	return m_BoundSphere;
}

const CWorldBoundBox & CActorBase::GetWorldBoundBox() const
{
	return m_WorldBoundBox;
}

const CWorldBoundSphere & CActorBase::GetWorldBoundSphere() const
{
	return m_WorldBoundSphere;
}

void CActorBase::RenderVisualizer()
{
	m_pShape->RenderVisualizer(vec4(1.0f, 0.0f, 0.0f, 1.0f));
	g_Engine.pVisualizer->RenderVector(m_pShape->GetCenter(), m_pShape->GetCenter() + m_vDirection, vec4(1.0f, 0.0f, 0.0f, 1.0f));
}

void CActorBase::SetFriction(float friction)
{
	m_fFriction = friction;
}

float CActorBase::GetFriction() const
{
	return m_fFriction;
}

void CActorBase::SetPosition(const MathLib::vec3& pos)
{
	m_vPosition = pos;
}

const MathLib::vec3& CActorBase::GetPosition() const
{
	return m_vPosition;
}
