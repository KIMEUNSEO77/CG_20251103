#include <glew.h>
#include <freeglut.h>
#include <freeglut_ext.h> 
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "filetobuf.h"
#include "shaderMaker.h"
#include "obj_load.h"

void make_vertexShaders();
void make_fragmentShaders();
GLuint make_shaderProgram();
GLvoid drawScene();
GLvoid Reshape(int w, int h);

CubeMesh gCube;

// 면 색상 
static const glm::vec3 kFaceColors[6] =
{
	{0.784f, 0.635f, 0.784f}, {0.564f, 0.933f, 0.564f}, {1.0f, 0.7f, 0.3f},
	{1.0f, 0.713f, 0.756f}, {1,0,1}, {0.678f, 0.847f, 0.902f}
};

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);  // 깊이 버퍼 추가
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(width, height);
	glutCreateWindow("Tesk_22");

	glewExperimental = GL_TRUE;
	glewInit();

	glutDisplayFunc(drawScene);
	glutReshapeFunc(Reshape);

	glEnable(GL_DEPTH_TEST); // 깊이 테스트 활성화
	glEnable(GL_CULL_FACE);  // 면 제거 활성화
	glCullFace(GL_FRONT);     // 앞면 제거

	if (!LoadOBJ_PosNorm_Interleaved("unit_cube.obj", gCube))
	{
		std::cerr << "Failed to load cube.obj\n";
		return 1;
	}

	make_vertexShaders();
	make_fragmentShaders();
	shaderProgramID = make_shaderProgram();

	glutMainLoop();

	return 0;
}

// 큐브 그리는 함수
void DrawCube(const CubeMesh& mesh, GLuint shaderProgram, const glm::mat4& model, const glm::vec3& color)
{
	GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);

	GLint colorLoc = glGetUniformLocation(shaderProgram, "vColor");
	//glUniform3fv(colorLoc, 1, &color[0]);

	glBindVertexArray(mesh.vao);
	for (size_t i = 0; i < mesh.faceRanges.size(); ++i)
	{
		const auto& r = mesh.faceRanges[i];

		// 면별 색 지정
		const glm::vec3 c = kFaceColors[i % 6];
		glUniform3f(colorLoc, c.r, c.g, c.b);

		glDrawArrays(GL_TRIANGLES, r.first, r.count);
	}
	glBindVertexArray(0);
}

GLvoid drawScene()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(shaderProgramID);

	GLint viewLoc = glGetUniformLocation(shaderProgramID, "view");
	GLint projLoc = glGetUniformLocation(shaderProgramID, "projection");

	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 8.0f);
	glm::vec3 cameraDirection = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	glm::mat4 vTransform = glm::mat4(1.0f);
	vTransform = glm::lookAt(cameraPos, cameraDirection, cameraUp);
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &vTransform[0][0]);

	glm::mat4 pTransform = glm::mat4(1.0f);
	pTransform = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, &pTransform[0][0]);

	// 큐브 그리기
	// 공통
	glm::mat4 share = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
	share = glm::translate(share, glm::vec3(-0.5f, 0.0f, -5.0f));
	// 큐브 그리기
	// 중심 (-0.5, 0, -5) 한변의 길이 5
	glm::mat4 centerCube = share * glm::scale(glm::mat4(1.0f), glm::vec3(5.0f, 5.0f, 5.0f));
	DrawCube(gCube, shaderProgramID, centerCube, glm::vec3(0.678f, 0.847f, 0.902f));


	glutSwapBuffers();
}

GLvoid Reshape(int w, int h)
{
	width = w;
	height = h;
	glViewport(0, 0, w, h);
}