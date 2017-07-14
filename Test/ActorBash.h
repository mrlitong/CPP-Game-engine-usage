#pragma once
#ifndef __ACTOR_BASE_H__
#define __ACTOR_BASE_H__

#include "Shape.h"
#include "Bounds.h"
#include "Player.h"
/*
 */
class CBRObject;
class CBodyDummy;
class CBodyRigid;
class CObjectDummy;
class CShapeCapsule;

/*
 *������ײ����ײ�뾶����ײ�߶ȵȣ�������
 */
class CActorBase
{    
public:
	CActorBase();
	virtual ~CActorBase();

	void SetEnabled(int enable);
	int IsEnabled() const;

	void Update(float ifps);

	// intersection mask
	void SetIntersectionMask(int mask);
	int GetIntersectionMask() const;

	    // collision
    void SetCollision(int collision);
    int GetCollision() const;
        
    // collision mask
    void SetCollisionMask(int mask);
    int GetCollisionMask() const;
        
    // collision radius
    void SetCollisionRadius(float radius);
    float GetCollisionRadius() const;
        
    // collision height
    void SetCollisionHeight(float height);
    float GetCollisionHeight() const;
        
    // maximum friction	���Ħ����ֵ
    void SetFriction(float friction);
    float GetFriction() const;
        
    // minimum velocity	��С�ٶ�
    void SetMinVelocity(float velocity);
    float GetMinVelocity() const;
        
    // maximum velocity ����ٶ�
    void SetMaxVelocity(float velocity);
    float GetMaxVelocity() const;

	// acceleration ���ٶ�
    void SetAcceleration(float acceleration);
    float GetAcceleration() const;
        
    // damping	��б
    void SetDamping(float damping);
    float GetDamping() const;
        
    // jumping ��Ծ
    void SetJumping(float jumping);
    float GetJumping() const;
  
    // view direction ���߳���
    void SetViewDirection(const MathLib::vec3 &direction);
    const MathLib::vec3 &GetViewDirection() const;
    //position λ��
    void SetPosition( const MathLib::vec3 & pos );
    const MathLib::vec3& GetPosition() const;
	



	enum {            //״̬
		STATE_FORWARD = 0,
		STATE_BACKWARD,
		STATE_MOVE_LEFT,
		STATE_MOVE_RIGHT,
		STATE_CROUCH,	//�׷�
		STATE_JUMP,
		STATE_RUN,
		NUM_STATES,
	};
	    // state status		//״̬�Ƿ���ã�״̬��ʼ������
    enum {
        STATE_DISABLED = 0,
        STATE_ENABLED,
        STATE_BEGIN,
        STATE_END,
    };

	    // current state
    int GetState(int state) const;
    float GetStateTime(int state) const;
        
    // contacts
    int GetNumContacts() const;
    const CShape::Contact &GetContact(int num) const;
        
    // ground flag	//���ƶ�����͵�λ�ñ�־
    void SetGround(int ground);
    int GetGround() const;
        
    // ceiling flag	//���ƶ�����ߵ�λ�ñ�־
    void SetCeiling(int ceiling);
    int GetCeiling() const;
        
    // bounds Լ��
    virtual const CBoundBox &GetBoundBox() const;
    virtual const CBoundSphere &GetBoundSphere() const;
    //ͬ��
    virtual const CWorldBoundBox &GetWorldBoundBox() const;
    virtual const CWorldBoundSphere &GetWorldBoundSphere() const;

    virtual void  RenderVisualizer();
private:
	    // update bounds	//���±߽�
    void Update_Bounds();
        
    // player transformation
    MathLib::Mat4 Get_Body_Transform() const;
   
    // update states
    int Update_State(int condition,int state,int begin,int end,float ifps);
    void Update_States(int enabled,float ifps);
        
    int m_nFrame;                   // frame number
    
    MathLib::vec3   m_vUp;          // up  ��ά�����е�����
    MathLib::vec3   m_vVelocity;    // velocity vector	��ǰ�ٶ�����

    CObjectDummy *m_pObject;        // dummy object
    CBodyDummy *m_pDummy;           // dummy body
        
    int m_nCollision;               // collision flag
    CShapeCapsule *m_pShape;        // collision shape
 
    float m_fFriction;              // fraction when frozen
    float m_fMinVelocity;           // minimum velocity
    float m_fMaxVelocity;           // maximum velocity
    float m_fAcceleration;          // acceleration
    float m_fDamping;               // damping
    float m_fJumping;               // jumping
        
    int m_nFlush;                   // flush flag
    MathLib::Vec3 m_vPosition;      // position
    MathLib::vec3 m_vDirection;     // direction
    float m_fPhiAngle;              // phi angle
    float m_fThetaAngle;            // theta angle
        
    int m_pStates[NUM_STATES];      // state vector
    float m_pTimes[NUM_STATES];     // time vector
        
    int m_nGround;                  // ground flag
    int m_nCeiling;                 // ceiling flag
        
    CBoundBox m_BoundBox;           // bounding box
    CBoundSphere m_BoundSphere;     // bounding sphere

    CWorldBoundBox m_WorldBoundBox;           // bounding box
    CWorldBoundSphere m_WorldBoundSphere;     // bounding sphere

    CVector<CShape::Contact> m_vecContacts;
    int m_nEnable;


};
#endif  __PLAYER_ACTOR_H__ 