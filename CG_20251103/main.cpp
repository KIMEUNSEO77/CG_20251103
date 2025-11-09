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

Mesh gTank;
float moveX = 0.0f; float moveZ = 0.0f;
bool middleRotatingY = false;
float angleY = 0.0f;

bool rotatingCameraZ = false;
float angleCameraZ = 0.0f;
int dirCameraZ = 1;   // 1: 양, -1: 음

bool rotatingCameraX = false;
float angleCameraX = 0.0f;
int dirCameraX = 1;   // 1: 양, -1: 음

bool rotatingCameraY = false;
float angleCameraY = 0.0f;

bool rotatingCameraCenterY = false;
float angleCameraCenterY = 0.0f;

bool rotatingBarel = false;  // 포신 회전
float angleBarel1 = 0.0f;    // 왼쪽 포신 각도
float angleBarel2 = 0.0f;    // 오른쪽 포신 각도

bool rotatingFlag = false;
float angleFlag1 = 0.0f;
float angleFlag2 = 0.0f;
int dirFlag = 1;
int dirFlag2 = -1;

bool changingPosition = false;  // top body 위치 변경
float animationTime = 0.0f;
glm::vec3 pos1 = glm::vec3(-0.7f, 0.4f, 0.0f);
glm::vec3 pos2 = glm::vec3(0.7f, 0.4f, 0.0f);
glm::vec3 purposePos1 = glm::vec3(0.7f, 0.4f, 0.0f);
glm::vec3 purposePos2 = glm::vec3(-0.7f, 0.4f, 0.0f);

bool rotatingAnimation = false;  // 카메라 공전 애니메이션
bool isCameraWaiting = false;
int cameraWaitCounter = 0;
const int cameraWaitFrames = 60; // 1초
float cameraStartAngle = 0.0f;
bool rotatingCenterY2 = false;

void Timer(int value)
{
	if (middleRotatingY) angleY += 1.0f;
	if (rotatingCameraZ) angleCameraZ += dirCameraZ * 2.0f;
	if (rotatingCameraX) angleCameraX += dirCameraX * 2.0f;
	if (rotatingCameraY) angleCameraY += 2.0f;
	if (rotatingCameraCenterY) angleCameraCenterY += 5.0f;
	if (rotatingCenterY2) angleCameraCenterY += 2.0f;
	if (rotatingBarel)
	{
		angleBarel1 -= 2.0f;
		angleBarel2 += 2.0f;
	}

	if (rotatingFlag)
	{
		angleFlag1 += 2.0f * dirFlag;
		if (angleFlag1 >= 90.0f) dirFlag = -1;
		else if (angleFlag1 <= -90.0f) dirFlag = 1;
		angleFlag2 += 2.0f * dirFlag2;
		if (angleFlag2 >= 90.0f) dirFlag2 = -1;
		else if (angleFlag2 <= -90.0f) dirFlag2 = 1;
	}

	if (changingPosition)
	{
		animationTime += 0.01f;

		if (animationTime >= 1.0f)
		{
			pos1 = purposePos1;
			pos2 = purposePos2;

			glm::vec3 temp = purposePos1;
			purposePos1 = purposePos2;
			purposePos2 = temp;

			animationTime = 0.0f;
		}

		pos1 = glm::mix(purposePos2, purposePos1, animationTime);
		pos2 = glm::mix(purposePos1, purposePos2, animationTime);

	}

	// 공전 애니메이션
	if (rotatingAnimation)
	{
		if (!isCameraWaiting)
		{
			rotatingCameraCenterY = true;
			angleCameraCenterY += 2.0f;
			// 한 바퀴 돌았는지 체크
			if (angleCameraCenterY - cameraStartAngle >= 360.0f)
			{
				angleCameraCenterY = cameraStartAngle + 360.0f;
				rotatingCameraCenterY = false;
				isCameraWaiting = true;
				cameraWaitCounter = 0;
				cameraStartAngle = angleCameraCenterY; // 다음 바퀴 시작점
			}
		}
		else
		{
			// 대기 상태
			rotatingCameraCenterY = false;
			cameraWaitCounter++;
			if (cameraWaitCounter >= cameraWaitFrames)
			{
				isCameraWaiting = false;
				// 다음 바퀴 시작
			}
		}
	}
	else
	{
		rotatingCameraCenterY = false;
		isCameraWaiting = false;
		cameraWaitCounter = 0;
		cameraStartAngle = angleCameraCenterY;
	}

	glutPostRedisplay();
	glutTimerFunc(16, Timer, 0);
}

void StopAllRotations()
{
	middleRotatingY = false;
	rotatingCameraZ = false;
	rotatingCameraX = false;
	rotatingCameraY = false;
	rotatingCameraCenterY = false;
	rotatingBarel = false;
	rotatingFlag = false;
	changingPosition = false;

	glutPostRedisplay();
}


void Reset()
{
	middleRotatingY = false;
	angleY = 0.0f;
	rotatingCameraZ = false;
	angleCameraZ = 0.0f;
	rotatingCameraX = false;
	angleCameraX = 0.0f;
	rotatingCameraY = false;
	angleCameraY = 0.0f;
	rotatingCameraCenterY = false;
	angleCameraCenterY = 0.0f;
	moveX = 0.0f; moveZ = 0.0f;

	rotatingBarel = false;
	angleBarel1 = 0.0f; angleBarel2 = 0.0f;
	rotatingFlag = false;
	angleFlag1 = 0.0f; angleFlag2 = 0.0f;
	dirFlag = 1; dirFlag2 = -1;
	changingPosition = false;
	animationTime = 0.0f;
	pos1 = glm::vec3(-0.7f, 0.4f, 0.0f); pos2 = glm::vec3(0.7f, 0.4f, 0.0f);
	purposePos1 = glm::vec3(0.7f, 0.4f, 0.0f); purposePos2 = glm::vec3(-0.7f, 0.4f, 0.0f);

	glutPostRedisplay();
}

GLvoid Keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 't': middleRotatingY = !middleRotatingY; break;
	case 'z': rotatingCameraZ = !rotatingCameraZ; dirCameraZ = 1; rotatingCameraX = false; rotatingCameraY = false; break;
	case 'Z': rotatingCameraZ = !rotatingCameraZ; dirCameraZ = -1; rotatingCameraX = false; rotatingCameraY = false; break;
	case 'x': rotatingCameraX = !rotatingCameraX; dirCameraX = 1; rotatingCameraZ = false; rotatingCameraY = false; break;
	case 'X': rotatingCameraX = !rotatingCameraX; dirCameraX = -1; rotatingCameraZ = false; rotatingCameraY = false; break;
	case 'y': rotatingCameraY = !rotatingCameraY; rotatingCameraX = false; rotatingCameraZ = false; break;
	case 'r': rotatingCenterY2 = !rotatingCenterY2; rotatingCameraX = false; rotatingCameraZ = false; rotatingCameraY = false; break;
	case 'g': rotatingBarel = !rotatingBarel; break;
	case 'p': rotatingFlag = !rotatingFlag; break;
	case 'i': changingPosition = !changingPosition; break;
	case 'a': rotatingAnimation = !rotatingAnimation; break;
	case 'o': StopAllRotations(); break;
	case 'c': Reset(); break;
	case 'q': exit(0); break;
	}
}

void SpecialKeyboard(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_UP: moveZ -= 0.1f; break;
	case GLUT_KEY_DOWN: moveZ += 0.1f; break;
	case GLUT_KEY_LEFT: moveX -= 0.1f; break;
	case GLUT_KEY_RIGHT: moveX += 0.1f; break;
	}
	glutPostRedisplay();
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);  // 깊이 버퍼 추가
	glutInitWindowPosition(50, 50);
	glutInitWindowSize(width, height);
	glutCreateWindow("Tesk_23");

	glutDisplayFunc(drawScene);
	glutReshapeFunc(Reshape);

	glewExperimental = GL_TRUE;
	glewInit();

	glEnable(GL_DEPTH_TEST); // 깊이 테스트 활성화
	glEnable(GL_CULL_FACE);  // 은면 제거 활성화

	make_vertexShaders();
	make_fragmentShaders();
	shaderProgramID = make_shaderProgram();

	if (!LoadOBJ_PosNorm_Interleaved("unit_cube.obj", gTank)) {
		std::cerr << "Failed to load unit_cube.obj\n";
		return 1;
	}

	glutKeyboardFunc(Keyboard);
	glutSpecialFunc(SpecialKeyboard);
	glutTimerFunc(0, Timer, 0);

	glutMainLoop();
	return 0;
}

void DrawCube(const Mesh& mesh, GLuint shaderProgram, const glm::mat4& model, const glm::vec3& color)
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

	int viewLeft[3] = { 0, 500, 500 };
	int viewBottom[3] = { 0, 450, 0 };
	int viewWidth = width / 2;
	int viewHeight = height;

	for (int i = 0; i < 3; ++i)
	{
		if (i == 0) viewHeight = height;
		else viewHeight = height / 2;

		glViewport(viewLeft[i], viewBottom[i], viewWidth, viewHeight);

		GLint viewLoc = glGetUniformLocation(shaderProgramID, "view");
		GLint projLoc = glGetUniformLocation(shaderProgramID, "projection");

		glm::vec3 cameraPos, cameraTarget, cameraUp;
		glm::mat4 vTransform, pTransform;

		float radX = glm::radians(angleCameraX);
		float radY = glm::radians(angleCameraY);
		float radZ = glm::radians(angleCameraZ);

		float radiusCenterY = 8.0f;
		float radCenterY = glm::radians(angleCameraCenterY);

		if (i == 0)
		{
			float radius = 8.0f;
			cameraPos = glm::vec3(0.0f, 0.0f, radius);

			if (rotatingCameraCenterY || rotatingCenterY2)
			{
				// y축 공전: 카메라가 y축을 중심으로 원을 그림
				cameraPos = glm::vec3(
					radiusCenterY * sin(radCenterY),
					0.0f,
					radiusCenterY * cos(radCenterY)
				);
				cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f); // 원점 바라봄
				//cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
			}


			// X, Y축 회전 적용
			glm::mat4 rotX = glm::mat4(1.0f);
			rotX = glm::rotate(rotX, glm::radians(-15.0f), glm::vec3(1, 0, 0)) *
				glm::rotate(rotX, glm::radians(radX), glm::vec3(1, 0, 0));
			glm::mat4 rotYmat = glm::mat4(1.0f);
			rotYmat = glm::rotate(rotYmat, glm::radians(radY), glm::vec3(0, 1, 0));
			cameraPos = glm::vec3(rotYmat * rotX * glm::vec4(cameraPos, 1.0f));

			cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);

			// Z축 회전(up벡터 회전)
			cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
			glm::vec3 dir = glm::normalize(cameraTarget - cameraPos);
			glm::mat4 rollMat = glm::rotate(glm::mat4(1.0f), radZ, dir);
			cameraUp = glm::vec3(rollMat * glm::vec4(cameraUp, 0.0f));

			vTransform = glm::lookAt(cameraPos, cameraTarget, cameraUp);
			pTransform = glm::perspective(glm::radians(45.0f), (float)viewWidth / (float)viewHeight, 0.1f, 100.0f);
		}
		else if (i == 1)
		{
			// xz 평면
			float radius = 8.0f;
			cameraPos = glm::vec3(0.0f, radius, 0.0f);

			// Y축 회전
			glm::mat4 rotYmat = glm::rotate(glm::mat4(1.0f), radY, glm::vec3(0, 1, 0));
			cameraPos = glm::vec3(rotYmat * glm::vec4(cameraPos, 1.0f));

			cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);

			// up벡터는 -z, Z축 회전 적용
			cameraUp = glm::vec3(0.0f, 0.0f, -1.0f);
			glm::vec3 dir = glm::normalize(cameraTarget - cameraPos);
			glm::mat4 rollMat = glm::rotate(glm::mat4(1.0f), radZ, dir);
			cameraUp = glm::vec3(rollMat * glm::vec4(cameraUp, 0.0f));

			vTransform = glm::lookAt(cameraPos, cameraTarget, cameraUp);
			pTransform = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);
		}
		else
		{
			float radius = 8.0f;
			cameraPos = glm::vec3(0.0f, 0.0f, radius);

			glm::mat4 rotX = glm::mat4(1.0f);
			rotX = glm::rotate(rotX, glm::radians(radX), glm::vec3(1, 0, 0));
			glm::mat4 rotYmat = glm::mat4(1.0f);
			rotYmat = glm::rotate(rotYmat, glm::radians(radY), glm::vec3(0, 1, 0));
			cameraPos = glm::vec3(rotYmat * rotX * glm::vec4(cameraPos, 1.0f));

			cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);

			cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
			glm::vec3 dir = glm::normalize(cameraTarget - cameraPos);
			glm::mat4 rollMat = glm::rotate(glm::mat4(1.0f), radZ, dir);
			cameraUp = glm::vec3(rollMat * glm::vec4(cameraUp, 0.0f));

			if (rotatingCameraCenterY || rotatingCenterY2)
			{
				// y축 공전: 카메라가 y축을 중심으로 원을 그림
				cameraPos = glm::vec3(
					radiusCenterY * sin(radCenterY),
					0.0f,
					radiusCenterY * cos(radCenterY)
				);
				cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f); // 원점 바라봄
				cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
			}

			vTransform = glm::lookAt(cameraPos, cameraTarget, cameraUp);
			pTransform = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);
		}

		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &vTransform[0][0]);	
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, &pTransform[0][0]);

		glm::mat4 ground = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.5f, 0.0f));
		ground = glm::scale(ground, glm::vec3(100.0f, 0.5f, 100.0f));
		DrawCube(gTank, shaderProgramID, ground, glm::vec3(1.0f, 0.713f, 0.756f));

		glm::mat4 M_tank = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));			
		M_tank = glm::translate(M_tank, glm::vec3(moveX, 0.0f, moveZ));
		glm::mat4 bottomBody = M_tank * glm::scale(glm::mat4(1.0f), glm::vec3(3.0f, 0.5f, 1.0f));
		DrawCube(gTank, shaderProgramID, bottomBody, glm::vec3(0.678f, 0.847f, 0.902f));

		glm::mat4 M_turret = M_tank;
		M_turret = glm::translate(M_turret, glm::vec3(0.0f, 0.4f, 0.0f));
		M_turret = glm::rotate(M_turret, glm::radians(angleY), glm::vec3(0.0f, 1.0f, 0.0f));

		glm::mat4 middleBody = M_turret * glm::scale(glm::mat4(1.0f), glm::vec3(1.5f, 0.25f, 0.5f));
		DrawCube(gTank, shaderProgramID, middleBody, glm::vec3(0.564f, 0.933f, 0.564f));

		glm::mat4 M_top1 = M_turret * glm::translate(glm::mat4(1.0f), pos1);
		glm::mat4 topBody1 = M_top1 * glm::scale(glm::mat4(1.0f), glm::vec3(0.75f, 0.5f, 0.5f));
		DrawCube(gTank, shaderProgramID, topBody1, glm::vec3(0.784f, 0.635f, 0.784f));

		glm::mat4 M_top2 = M_turret * glm::translate(glm::mat4(1.0f), pos2);
		glm::mat4 topBody2 = M_top2 * glm::scale(glm::mat4(1.0f), glm::vec3(0.75f, 0.5f, 0.5f));
		DrawCube(gTank, shaderProgramID, topBody2, glm::vec3(0.784f, 0.635f, 0.784f));

		glm::mat4 flag1 = M_top1
			* glm::rotate(glm::mat4(1.0f), glm::radians(angleFlag1), glm::vec3(1.0f, 0.0f, 0.0f))
			* glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.6f, 0.0f))
			* glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 1.0f, 0.1f));
		DrawCube(gTank, shaderProgramID, flag1, glm::vec3(1.0f, 0.7f, 0.3f));

		glm::mat4 flag2 = M_top2
			* glm::rotate(glm::mat4(1.0f), glm::radians(angleFlag2), glm::vec3(1.0f, 0.0f, 0.0f))
			* glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.6f, 0.0f))
			* glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 1.0f, 0.1f));
		DrawCube(gTank, shaderProgramID, flag2, glm::vec3(1.0f, 0.7f, 0.3f));

		glm::mat4 barrel1 = M_top1
			* glm::rotate(glm::mat4(1.0f), glm::radians(angleBarel1), glm::vec3(0.0f, 1.0f, 0.0f))
			* glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.2f, 0.5f))
			* glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.1f, 1.0f));
		DrawCube(gTank, shaderProgramID, barrel1, glm::vec3(0.5f, 0.0f, 0.5f));

		glm::mat4 barrel2 = M_top2
			* glm::rotate(glm::mat4(1.0f), glm::radians(angleBarel2), glm::vec3(0.0f, 1.0f, 0.0f))
			* glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.2f, 0.5f))
			* glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.1f, 1.0f));
		DrawCube(gTank, shaderProgramID, barrel2, glm::vec3(0.5f, 0.0f, 0.5f));
	}

	glutSwapBuffers();
}

GLvoid Reshape(int w, int h)
{
	width = w;
	height = h;
	glViewport(0, 0, w, h);
}