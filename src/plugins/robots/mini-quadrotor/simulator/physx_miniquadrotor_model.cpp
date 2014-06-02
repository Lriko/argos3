/**
 * @file <argos3/plugins/robots/mini-quadrotor/simulator/physx_miniquadrotor_model.cpp>
 *
 * @author Carlo Pinciroli - <ilpincy@gmail.com>
 */

#include "physx_miniquadrotor_model.h"
#include <argos3/plugins/robots/mini-quadrotor/simulator/miniquadrotor_entity.h>
#include <argos3/plugins/simulator/entities/rotor_equipped_entity.h>

namespace argos {

   /****************************************/
   /****************************************/
   
   /*
    * The parameter values are taken from data provided by Kumar's group directly
    */
   static const Real ARM_LENGTH            = 0.063f;
   static const Real ARM_HALF_LENGTH       = ARM_LENGTH * 0.5f;
   static const Real PROPELLER_LENGTH      = 0.082f;
   static const Real PROPELLER_HALF_LENGTH = PROPELLER_LENGTH * 0.5f;
   static const Real BODY_HALF_WIDTH       = ARM_HALF_LENGTH + PROPELLER_HALF_LENGTH;
   static const Real BODY_HEIGHT           = 0.015f;
   static const Real BODY_HALF_HEIGHT      = BODY_HEIGHT * 0.5f;
   static const Real BODY_MASS             = 0.06736f;
   static const Real UPLIFT_COEFFICIENT    = 1.9985e-9;
   static const Real DRAG_COEFFICIENT      = 4.737e-12;

   static const physx::PxVec3 POSITION_ERROR_COEFF(6.61f, 6.61f, 72.0f); // unused
   static const physx::PxVec3 VELOCITY_ERROR_COEFF(5.14f, 5.14f, 24.0f); // unused

   static const physx::PxTransform PITCH_ARM_POSE(physx::PxQuat(physx::PxHalfPi, physx::PxVec3(0.0f, 0.0f, 1.0f)));
   static const physx::PxVec3      INERTIA_TENSOR_DIAGONAL(2.32e-3, 2.32e-3, 4e-3);
   static const physx::PxMat33     INERTIA_TENSOR(physx::PxMat33::createDiagonal(INERTIA_TENSOR_DIAGONAL));
   static const physx::PxMat33     INERTIA_TENSOR_INVERSE(INERTIA_TENSOR.getInverse());

   /****************************************/
   /****************************************/

   CPhysXMiniQuadrotorModel::CPhysXMiniQuadrotorModel(CPhysXEngine& c_engine,
                                                      CMiniQuadrotorEntity& c_entity) :
      CPhysXModel(c_engine, c_entity.GetEmbodiedEntity()),
      m_cMiniQuadrotorEntity(c_entity) {
      /* Calculate base center */
      m_cBaseCenterLocal.x = 0.0f;
      m_cBaseCenterLocal.y = 0.0f;
      m_cBaseCenterLocal.z = -BODY_HALF_HEIGHT;
      /* Get position and orientation in this engine's representation */
      physx::PxVec3 cPos;
      CVector3ToPxVec3(GetEmbodiedEntity().GetPosition(), cPos);
      physx::PxQuat cOrient;
      CQuaternionToPxQuat(GetEmbodiedEntity().GetOrientation(), cOrient);
      /* Create the transform
       * 1. a translation up by half body height
       * 2. a rotation around the base
       * 3. a translation to the final position
       */
      physx::PxTransform cTranslation1(physx::PxVec3(0.0f, 0.0f, BODY_HALF_HEIGHT));
      physx::PxTransform cRotation(cOrient);
      physx::PxTransform cTranslation2(cPos);
      physx::PxTransform cFinalTrans = cTranslation2 * cRotation * cTranslation1;
      /* Create the capsule geometry for the arms */
      m_pcArmGeometry = new physx::PxCapsuleGeometry(BODY_HALF_HEIGHT,
                                                     BODY_HALF_WIDTH);
      /* Create the body in its initial position and orientation */
      m_pcBody = GetPhysXEngine().GetPhysics().createRigidDynamic(cFinalTrans);
      /* Enable CCD on the body */
      m_pcBody->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_CCD, true);
      /* Create the shape for the roll arm */
      m_pcRollArmShape = m_pcBody->createShape(*m_pcArmGeometry,
                                               GetPhysXEngine().GetDefaultMaterial());
      /* Assign the user data pointer to this model */
      m_pcRollArmShape->userData = this;
      /* Create the shape for the pitch arm */
      m_pcPitchArmShape = m_pcBody->createShape(*m_pcArmGeometry,
                                                GetPhysXEngine().GetDefaultMaterial(),
                                                PITCH_ARM_POSE);
      /* Assign the user data pointer to this model */
      m_pcPitchArmShape->userData = this;
      /* Set body mass and inertia tensor */
      m_pcBody->setMass(BODY_MASS);
      m_pcBody->setMassSpaceInertiaTensor(INERTIA_TENSOR_DIAGONAL);
      /* Add body to the scene */
      GetPhysXEngine().GetScene().addActor(*m_pcBody);
      /* Calculate bounding box */
      CalculateBoundingBox();
   }

   /****************************************/
   /****************************************/

   CPhysXMiniQuadrotorModel::~CPhysXMiniQuadrotorModel() {
      GetPhysXEngine().GetScene().removeActor(*m_pcBody);
      m_pcBody->release();
   }

   /****************************************/
   /****************************************/

   bool CPhysXMiniQuadrotorModel::MoveTo(const CVector3& c_position,
                                         const CQuaternion& c_orientation,
                                         bool b_check_only) {
      return false;
   }

   /****************************************/
   /****************************************/

   void CPhysXMiniQuadrotorModel::Reset() {
   }

   /****************************************/
   /****************************************/

   void CPhysXMiniQuadrotorModel::CalculateBoundingBox() {
      physx::PxBounds3 cPxAABB(m_pcBody->getWorldBounds());
      PxVec3ToCVector3(cPxAABB.minimum, GetBoundingBox().MinCorner);
      PxVec3ToCVector3(cPxAABB.maximum, GetBoundingBox().MaxCorner);
   }

   /****************************************/
   /****************************************/

   void CPhysXMiniQuadrotorModel::UpdateEntityStatus() {
      /* Update bounding box */
      CalculateBoundingBox();
      /* Get the global pose of the object */
      physx::PxTransform cTrans = m_pcBody->getGlobalPose();
      /* Vector to the object base */
      /* Transform base */
      physx::PxVec3 cBaseGlobal(cTrans.rotate(m_cBaseCenterLocal));
      cBaseGlobal += cTrans.p;
      /* Set object position into ARGoS space */
      CVector3 cPos;
      PxVec3ToCVector3(cBaseGlobal, cPos);
      GetEmbodiedEntity().SetPosition(cPos);
      /* Set object orientation into ARGoS space */
      CQuaternion cOrient;
      PxQuatToCQuaternion(cTrans.q, cOrient);
      GetEmbodiedEntity().SetOrientation(cOrient);
   }

   /****************************************/
   /****************************************/

   void CPhysXMiniQuadrotorModel::UpdateFromEntityStatus() {
      /* Get desired rotor velocities */
      const Real* pfRotorVel = m_cMiniQuadrotorEntity.GetRotorEquippedEntity().GetRotorVelocities();
      /* Calculate the squares */
      Real pfSquareRotorVel[4] = {
         pfRotorVel[0] * pfRotorVel[0],
         pfRotorVel[1] * pfRotorVel[1],
         pfRotorVel[2] * pfRotorVel[2],
         pfRotorVel[3] * pfRotorVel[3]
      };
      /* Calculate uplift-related control input */
      Real fUpliftInput =
         UPLIFT_COEFFICIENT *
         ((pfSquareRotorVel[0]) +
          (pfSquareRotorVel[1]) +
          (pfSquareRotorVel[2]) +
          (pfSquareRotorVel[3]));
      /* Calculate torque-related control inputs */
      physx::PxVec3 cTorqueInputs(
         UPLIFT_COEFFICIENT * ARM_HALF_LENGTH * (pfSquareRotorVel[0] - pfSquareRotorVel[2]),
         UPLIFT_COEFFICIENT * ARM_HALF_LENGTH * (pfSquareRotorVel[1] - pfSquareRotorVel[3]),
         DRAG_COEFFICIENT * (pfSquareRotorVel[0] - pfSquareRotorVel[1] + pfSquareRotorVel[2] - pfSquareRotorVel[3]));
      /* Apply uplift */
      physx::PxRigidBodyExt::addLocalForceAtLocalPos(
         *m_pcBody,
         physx::PxVec3(0.0f, 0.0f, fUpliftInput),
         physx::PxVec3(0.0f));
      /* Apply rotational moment */
      physx::PxVec3 cTorque = (-m_pcBody->getAngularVelocity()).cross(INERTIA_TENSOR * m_pcBody->getAngularVelocity()) + cTorqueInputs;
      m_pcBody->addTorque(cTorque);
   }

   /****************************************/
   /****************************************/

   bool CPhysXMiniQuadrotorModel::IsCollidingWithSomething() const {
      return false;
   }

   /****************************************/
   /****************************************/

   REGISTER_STANDARD_PHYSX_OPERATIONS_ON_ENTITY(CMiniQuadrotorEntity, CPhysXMiniQuadrotorModel);

   /****************************************/
   /****************************************/

}
