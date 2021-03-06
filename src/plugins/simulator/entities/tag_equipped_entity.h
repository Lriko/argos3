/**
 * @file <argos3/plugins/simulator/entities/tag_equipped_entity.h>
 *
 * @author Michael Allwright - <allsey87@gmail.com>
 */

#ifndef TAG_EQUIPPED_ENTITY_H
#define TAG_EQUIPPED_ENTITY_H

namespace argos {
   class CTagEquippedEntity;
   class CTagEntity;
}

#include <argos3/core/simulator/entity/composable_entity.h>
#include <argos3/plugins/simulator/entities/tag_entity.h>
#include <vector>

namespace argos {

   /**
    * A container of CTagEntity.
    * <p>
    * This is a convenience class that acts a container of CTagEntity objects.
    * It is mostly useful when a robot is equipped with a number of tags, and you
    * want to manage them comfortably.
    * </p>
    * @see CTagEntity
    */
   class CTagEquippedEntity : public CComposableEntity {

   public:

      ENABLE_VTABLE();

   public:

      struct SInstance {
         CTagEntity& Tag;
         SAnchor& Anchor;
         CVector3 PositionOffset;
         CQuaternion OrientationOffset;
         SInstance(CTagEntity& c_tag,
                   SAnchor& s_anchor,
                   const CVector3& c_position_offset,
                   const CQuaternion& c_orientation_offset);
         using TVector = std::vector<SInstance>;
      };

   public:

      /**
       * Class constructor.
       * @param pc_parent The parent entity.
       */
      CTagEquippedEntity(CComposableEntity* pc_parent);

      /**
       * Class constructor.
       * @param pc_parent The parent entity.
       * @param str_id The id of this entity.
       */
      CTagEquippedEntity(CComposableEntity* pc_parent,
                         const std::string& str_id);

      virtual ~CTagEquippedEntity() {}

      virtual void Init(TConfigurationNode& t_tree);

      virtual void Reset();

      virtual void Enable();

      virtual void Disable();

      /**
       * Returns a tag by numeric index.
       * @param un_index The index of the specified tag.
       * @return A tag by numeric index.
       * @see GetTags()
       */
      CTagEntity& GetTag(UInt32 un_index);

      /**
       * Returns the tags.
       * @return The tags.
       * @see GetTag()
       */
      inline SInstance::TVector& GetInstances() {
         return m_vecInstances;
      }

      /**
       * Returns the tags as a const reference.
       * @return The tags as a const reference.
       * @see GetTag()
       */
      inline const SInstance::TVector& GetInstances() const {
         return m_vecInstances;
      }

      /**
       * Sets the payload of a tag.
       * @param un_index The index of the specified tag.
       * @param c_payload The payload of the tag.
       */
      void SetTagPayload(UInt32 un_index,
                         const std::string& str_payload);

      /**
       * Sets the payload of all the tags to the given payload.
       * @param c_payload The payload for the tags.
       * @see SetTagPayloads()
       */
      void SetTagPayloads(const std::string& str_payload);

      /**
       * Sets the payload of each tags to the payload given in the passed vector.
       * @param vec_payloads A vector containing the payload for the tags.
       * @see SetTagPayloads()
       * @throws CARGoSException if the size of the passed vector is different from
       * the number of tags.
       */
      void SetTagPayloads(const std::vector<std::string>& vec_payloads);

      /**
       * Adds the tags to the specified tag medium.
       * @param c_medium The tag medium.
       * @see CTagMedium
       */
      void AddToMedium(CTagMedium& c_medium);

      /**
       * Removes the tags from the associated tag medium.
       * @see CTagMedium
       */
      void RemoveFromMedium();

      virtual std::string GetTypeDescription() const {
         return "tags";
      }

   protected:

      virtual void UpdateComponents();

   protected:

      /** List of the tags managed by this entity */
      SInstance::TVector m_vecInstances;
   };

}

#endif
