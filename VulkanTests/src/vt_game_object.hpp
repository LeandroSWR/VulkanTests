#pragma once

#include "vt_model.hpp"

//libs
#include <glm/gtc/matrix_transform.hpp>
//std
#include <memory>
#include <unordered_map>

namespace vt {

	struct TransformComponent {
		glm::vec3 translation{}; // position offset
		glm::vec3 scale{ 1.f, 1.f , 1.f };
		glm::vec3 rotation;

        // Matrix corrsponds to Translate * Ry * Rx * Rz * Scale
        // Rotations correspond to Tait-bryan angles of Y(1), X(2), Z(3)
        // https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
		glm::mat4 mat4();
		glm::mat3 normalMatrix();
	};

	class VtGameObject {
	public:
		using id_t = unsigned int;
		using Map = std::unordered_map<id_t, VtGameObject>;

		static VtGameObject createGameObject() {
			static id_t currentId = 0;
			return VtGameObject{ currentId++ };
		}

		VtGameObject(const VtGameObject&) = delete;
		VtGameObject& operator=(const VtGameObject&) = delete;
		VtGameObject(VtGameObject&&) = default;
		VtGameObject& operator=(VtGameObject&&) = default;

		id_t getId() { return id; }

		std::shared_ptr<VtModel> model{};
		glm::vec3 color{};
		TransformComponent transform{};

	private:
		VtGameObject(id_t objId) : id{ objId } {}

		id_t id;
	};
}