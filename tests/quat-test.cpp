#define GLM_ENABLE_EXPERIMENTAL
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "gtest/gtest.h"
#include <glm/common.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <gtest/gtest.h>
#include <iostream>
#include <misc/slot-map.hpp>

struct Transform {
	glm::vec3 translate;
	glm::quat rotate;
	glm::vec3 scale;
};

TEST(QuatTest, RotationTest) {
	Transform camera_transform;
	glm::mat4x4 lkatmat = glm::lookAt(glm::vec3{ 0, 0, 0 }, { 5, 5, 5 }, { 0, 1, 0 });
	camera_transform.translate = { 5, 5, 5 };
	// auto q1 = glm::angleAxis(glm::radians(-135.0f), glm::vec3{ 0, 1, 0 });
	// auto q2 = glm::angleAxis(glm::radians(45.0f), glm::normalize(glm::vec3{ 1, 0, 1 }));
	camera_transform.rotate = glm::quatLookAt(glm::normalize(-camera_transform.translate), glm::vec3{ 0, 1, 0 }); // q1 * q2;
	glm::mat4x4 usrmat = glm::translate(glm::mat4(1), camera_transform.translate) * glm::mat4_cast(camera_transform.rotate);
	EXPECT_EQ(false, true) << "lookat : \n"
						   << glm::to_string(lkatmat) << std::endl;
	EXPECT_EQ(false, true) << " usr gen : \n"
						   << glm::to_string(usrmat) << std::endl;
	ASSERT_EQ(true, true);
}
