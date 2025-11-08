#include <glew.h>
#include <freeglut.h>
#include <freeglut_ext.h> 
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "filetobuf.h"
#include "shaderMaker.h"
#include "obj_load.h"
#include <random>
#include <vector>

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

struct Obstacle
{
	glm::vec3 position;
	glm::vec3 scale;
	glm::vec3 color;
};
std::vector<Obstacle> gObstacles(3);

bool openShield = false; float shieldY = 0.0f;
bool closeShield = false;

float randomFloat(float a, float b)
{
	std::random_device rd;
	std::mt19937 gen(rd());  // Mersenne Twister 엔진
	std::uniform_real_distribution<float> dist(a, b);

	return dist(gen);
}

// 장애물 초기화
void InitObstacles()
{
	for (int i = 0; i < gObstacles.size(); ++i)
	{
		gObstacles[i].position = glm::vec3(randomFloat(-3.0f, 3.0f), -3.0f, 
			randomFloat(-3.0f, 3.0f));
		gObstacles[i].scale = glm::vec3(randomFloat(0.2f, 1.5f));
		gObstacles[i].color = glm::vec3(randomFloat(0.0f, 1.0f), randomFloat(0.0f, 1.0f), 
			randomFloat(0.0f, 1.0f));
	}
}

void Timer(int value)
{
	if (openShield)
	{
		shieldY += 0.02f;
		if (shieldY >= 7.5f)
		{
			shieldY = 7.5f;
			openShield = false;
		}
	}
	if (closeShield)
	{
		shieldY -= 0.02f;
		if (shieldY <= 0.0f)
		{
			shieldY = 0.0f;
			closeShield = false;
		}
	}

	glutPostRedisplay();
	glutTimerFunc(16, Timer, 0);
}

GLvoid Keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	
	case 'o':
		openShield = true; closeShield = false;
		break;
	case 'O':
		closeShield = true; openShield = false;
		break;
	}
}

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

	InitObstacles();  // 장애물 초기화

	if (!LoadOBJ_PosNorm_Interleaved("unit_cube.obj", gCube))
	{
		std::cerr << "Failed to load cube.obj\n";
		return 1;
	}

	make_vertexShaders();
	make_fragmentShaders();
	shaderProgramID = make_shaderProgram();

	glutKeyboardFunc(Keyboard);
	glutTimerFunc(16, Timer, 0);

	glutMainLoop();

	return 0;
}

// 무대 큐브 그리는 함수
void DrawCenterCube(const CubeMesh& mesh, GLuint shaderProgram, const glm::mat4& model, const glm::vec3& color)
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

// 일반 큐브
void DrawCube(const CubeMesh& mesh, GLuint shaderProgram, const glm::mat4& model, const glm::vec3& color)
{
	GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);

	GLint colorLoc = glGetUniformLocation(shaderProgram, "vColor");
	glUniform3fv(colorLoc, 1, &color[0]);

	glBindVertexArray(mesh.vao);
	glDrawArrays(GL_TRIANGLES, 0, mesh.count);
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
	share = glm::translate(share, glm::vec3(0.0f, 0.0f, -5.0f));
	// 큐브 그리기
	// 중심 (0, 0, -5) 한변의 길이 7
	glm::mat4 centerCube = share * glm::scale(glm::mat4(1.0f), glm::vec3(7.0f, 7.0f, 7.0f));
	DrawCenterCube(gCube, shaderProgramID, centerCube, glm::vec3(0.678f, 0.847f, 0.902f));

	glCullFace(GL_BACK);  // 뒷면 제거


	// 장애물 그리기
	for (const auto& obs : gObstacles)
	{
		glm::mat4 model = share;
		model = glm::translate(model, obs.position);
		model = glm::scale(model, obs.scale);
		DrawCube(gCube, shaderProgramID, model, obs.color);
	}

	// 로봇 그리기
	// 머리
	glm::mat4 robotHead = share;
	robotHead = glm::translate(robotHead, glm::vec3(0.0f, 0.0f, 0.0f));
	robotHead = glm::scale(robotHead, glm::vec3(1.0f, 1.0f, 1.0f));
	DrawCube(gCube, shaderProgramID, robotHead, glm::vec3(1.0f, 0.7f, 0.3f));

	// 코
	glm::mat4 robotNose = share;
	robotNose = glm::translate(robotNose, glm::vec3(0.0f, 0.0f, 0.5f));
	robotNose = glm::scale(robotNose, glm::vec3(0.2f, 0.2f, 0.3f));
	DrawCube(gCube, shaderProgramID, robotNose, glm::vec3(1.0f, 0.0f, 0.0f));

	// 몸통
	glm::mat4 robotBody = share;
	robotBody = glm::translate(robotBody, glm::vec3(0.0f, -1.0f, 0.0f));
	robotBody = glm::scale(robotBody, glm::vec3(1.0f, 1.5f, 0.5f));
	DrawCube(gCube, shaderProgramID, robotBody, glm::vec3(0.5f, 0.9f, 0.5f));

	// 왼팔
	glm::mat4 robotArmL = share;
	robotArmL = glm::translate(robotArmL, glm::vec3(-0.5f, -1.0f, 0.0f));
	robotArmL = glm::scale(robotArmL, glm::vec3(0.3f, 1.2f, 0.3f));
	DrawCube(gCube, shaderProgramID, robotArmL, glm::vec3(0.7f, 0.6f, 0.7f));
	// 오른팔
	glm::mat4 robotArmR = share;
	robotArmR = glm::translate(robotArmR, glm::vec3(0.5f, -1.0f, 0.0f));
	robotArmR = glm::scale(robotArmR, glm::vec3(0.3f, 1.2f, 0.3f));
	DrawCube(gCube, shaderProgramID, robotArmR, glm::vec3(0.3f, 0.4f, 0.3f));

	// 왼다리
	glm::mat4 robotLegL = share;
	robotLegL = glm::translate(robotLegL, glm::vec3(-0.2f, -2.5f, 0.0f));
	robotLegL = glm::scale(robotLegL, glm::vec3(0.4f, 2.5f, 0.4f));
	DrawCube(gCube, shaderProgramID, robotLegL, glm::vec3(0.8f, 0.5f, 0.5f));
	// 오른다리
	glm::mat4 robotLegR = share;
	robotLegR = glm::translate(robotLegR, glm::vec3(0.2f, -2.5f, 0.0f));
	robotLegR = glm::scale(robotLegR, glm::vec3(0.4f, 2.5f, 0.4f));
	DrawCube(gCube, shaderProgramID, robotLegR, glm::vec3(0.5f, 0.5f, 0.8f));

	// 가림막
	glm::mat4 shield = share;
	shield = glm::translate(shield, glm::vec3(0.0f, shieldY, 3.3f));
	shield = glm::scale(shield, glm::vec3(7.0f, 7.0f, 0.1f));
	DrawCube(gCube, shaderProgramID, shield, glm::vec3(0.7f, 0.7f, 0.7f));


	glCullFace(GL_FRONT);  // 앞면 제거

	glutSwapBuffers();
}

GLvoid Reshape(int w, int h)
{
	width = w;
	height = h;
	glViewport(0, 0, w, h);
}