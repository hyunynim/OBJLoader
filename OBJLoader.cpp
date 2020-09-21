#include<bits/stdc++.h>
#include<gl/freeglut.h>
#include<Windows.h>
#include<gl\GL.h>
#include<gl\GLU.h>
#include<random>
#include<opencv2/highgui.hpp>
#include<opencv2/core.hpp>
#include<opencv2/imgproc.hpp>
#include<thread>
using namespace cv;
using namespace std;


typedef long long ll;
typedef uniform_int_distribution<int> ud;
typedef uniform_int_distribution<ll> ud2;
random_device rd;
mt19937 gen(rd());
ud cameraGen(-360, 360);
ud scaleGen(10, 500);

const int width = 256;
const int height = 256;

GLuint obj;
float carrot;


//other functions and main
//.obj loader code
struct VERTEX {
	float x, y, z;
};
struct UV {
	float x, y;
};
struct FACE {
	int v[3], vt[3], vn[3];
};
vector<VERTEX> v(1), vn(1);
vector<UV> vt(1);
vector<FACE> f;

int cameraParameterX = 1, cameraParameterY = 1, cameraParameterZ = 1;
float scale = 1.0;

const float scaleStep = 0.1, scaleMin = 0.1, scaleMax = 10.0;
const int cameraStep = 30, cameraMin = -360, cameraMax = 360;

Mat referenceImage, texture;

void SetPerspective(int x, int y, int z, float scale);
bool RandomTransformation(Mat& texture, Mat& ref, ll& mDif, int& mX, int& mY, int& mZ, float& mScale);
void CheckAllTransformation(Mat& texture, Mat& ref, ll& mDif, int& mX, int& mY, int& mZ, float& mScale);
void Str2Face(char msg[], FACE& face, int idx);
void LoadObj(string fileName);
void InitLight();
void Texture2Mat(Mat& res);
void LoadReferenceImage(Mat& ref, string fileName);
void Step(int& param, const int step);
void Step(float& param, const float step);
void MyKeyboard(unsigned char key, int x, int y);
void MyDisplay();
void MyReshape(int w, int h);


void SetPerspective(int x, int y, int z, float sc) {
	cameraParameterX = x;
	cameraParameterY = y;
	cameraParameterZ = z;
	scale = sc;
	MyDisplay();
}
bool RandomTransformation(Mat& texture, Mat& ref, ll& mDif, int& mX, int& mY, int& mZ, float& mScale) {
	ll diff = 0;

	int xx, yy, zz;
	xx = cameraGen(gen);
	yy = cameraGen(gen);
	zz = cameraGen(gen);
	float sc = scaleGen(gen) / 100.0;

	SetPerspective(xx, yy, zz, sc);
	Texture2Mat(texture);

	for (int i = 0; i < width; ++i) {
		for (int j = 0; j < height; ++j) {
			ll p1 = texture.at<uchar>(i, j);
			ll p2 = ref.at<uchar>(i, j);

			diff += (p1 - p2) * (p1 - p2);
		}
	}
	printf("cur diff: %lld\n", diff);
	if (mDif > diff) {
		printf("Camera parameter is updated(%d %d %d %f // pref diff: %lld // cur diff: %lld)\n", xx, yy, zz, sc, mDif, diff);
		mDif = diff;
		mX = xx;
		mY = yy;
		mZ = zz;
		mScale = sc;
		return 1;
	}
	return 0;
}
void CheckAllTransformation(Mat& texture, Mat& ref, ll& mDif, int& mX, int& mY, int& mZ, float& mScale) {
	ll diff = 0;

	for (int xx = -200; xx <= 200; xx += cameraStep) {
		for (int yy = -200; yy <= 200; yy += cameraStep) {
			for (int zz = -200; zz <= 200; zz += cameraStep) {
				for (int sca = 10; sca <= 400; sca += scaleStep *100) {
					float sc = sca / 100.0;
					SetPerspective(xx, yy, zz, sc);
					Texture2Mat(texture);
					for (int i = 0; i < width; ++i) {
						for (int j = 0; j < height; ++j) {
							ll p1 = texture.at<uchar>(i, j);
							ll p2 = ref.at<uchar>(i, j);

							diff += (p1 - p2) * (p1 - p2);
						}
					}
					//printf("cur diff: %lld\n", diff);
					if (mDif > diff) {
						printf("Camera parameter is updated(%d %d %d %f // pref diff: %lld // cur diff: %lld)\n", xx, yy, zz, sc, mDif, diff);
						mDif = diff;
						mX = xx;
						mY = yy;
						mZ = zz;
						mScale = sc;
					}
				}
			}
		}
	}
}
void Str2Face(char msg[], FACE& face, int idx) {
	int res = -1;
	int cnt = 0;
	for (int i = 0; msg[i]; ++i) {
		if (msg[i] == '/') {
			if (cnt)
				face.vt[idx] = res;
			else
				face.v[idx] = res;
			res = -1;
			++cnt;
		}
		else {
			if (res == -1)
				res = 0;
			res = res * 10 + (msg[i] - '0');
		}
	}
	face.vn[idx] = res;
}
void LoadObj(string fileName = "target.obj") {
	FILE* fp = fopen(fileName.c_str(), "r");
	if (fp == NULL) {
		printf("File open Error\n");
		return;
	}
	char msg[1010];
	string str;
	while (~fscanf(fp, "%s", msg)) {
		str = msg;
		if (str == "v") {
			VERTEX vTmp;
			fscanf(fp, "%f %f %f", &vTmp.x, &vTmp.y, &vTmp.z);
			v.push_back(vTmp);
		}
		else if (str == "vt") {
			UV vTmp;
			fscanf(fp, "%f %f", &vTmp.x, &vTmp.y);
			vt.push_back(vTmp);
		}
		else if (str == "vn") {
			VERTEX vTmp;
			fscanf(fp, "%f %f %f", &vTmp.x, &vTmp.y, &vTmp.z);
			vn.push_back(vTmp);
		}
		else if (str == "f") {
			FACE fTmp;
			for (int i = 0; i < 3; ++i) {
				fscanf(fp, "%s", msg);
				Str2Face(msg, fTmp, i);
			}
			f.push_back(fTmp);
		}
	}
	obj = glGenLists(1);
	glColor3f(0.3f, 0.3f, 0.3f);
	glNewList(obj, GL_COMPILE);

	glPushMatrix();
	for (int i = 0; i < f.size(); ++i) {
		glBegin(GL_TRIANGLES);
		for (int j = 0; j < 3; ++j)
			glVertex3f(v[f[i].v[j]].x, v[f[i].v[j]].y, v[f[i].v[j]].z);
		glEnd();
	}

	glPopMatrix();
	glEndList();
}
void InitLight() {
	GLfloat mat_diffuse[] = { 0.5,0.4,0.3,1.0 };
	GLfloat mat_specular[] = { 1.0,1.0,1.0,1.0 };
	GLfloat mat_ambient[] = { 0.5,0.4,0.3,1.0 };
	GLfloat mat_shininess[] = { 15.0 };
	GLfloat light_specular[] = { 1.0,1.0,1.0,1.0 };
	GLfloat light_diffuse[] = { 0.8,0.8,0.8,1.0 };
	GLfloat light_ambient[] = { 0.3,0.3,0.3,1.0 };
	GLfloat light_position[] = { -3,6,3.0,0.0 };
	glShadeModel(GL_SMOOTH);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
}
void Texture2Mat(Mat& res) {
	res.create(Size(width, height), CV_8UC3);

	glPixelStorei(GL_PACK_ALIGNMENT, (res.step & 3) ? 1 : 4);
	glPixelStorei(GL_PACK_ROW_LENGTH, res.step / res.elemSize());
	glReadPixels(0, 0, res.cols, res.rows, GL_BGR_EXT, GL_UNSIGNED_BYTE, res.data);

	cv::flip(res, res, 0);
}
void LoadReferenceImage(Mat& ref, string fileName = "ref.png") {
	ref = imread(fileName, IMREAD_GRAYSCALE);
	for (auto it = ref.begin<uchar>(); it != ref.end<uchar>(); ++it)
		if (*it != 255)* it = 80;

}
void Step(int& param, const int step) {
	param += step;
	if (step < 0)
		param = max(cameraMin, param);
	else
		param = min(cameraMax, param);
}
void Step(float& param, const float step) {
	param += step;
	if (step < 0)
		param = max(scaleMin, param);
	else
		param = min(scaleMax, param);
}

void GetDiff() {
	static ll diff = 1e18;
	static int xx, yy, zz;
	static float sc;
	for (int i = 0; i < 10; ++i) {
		if (RandomTransformation(texture, referenceImage, diff, xx, yy, zz, sc)) {
			SetPerspective(xx, yy, zz, sc);
			Texture2Mat(texture);
			imshow("Best image", texture);
		}
	}
	imshow("Reference", referenceImage);
}
void MyKeyboard(unsigned char key, int x, int y) {
	//printf("Press %c \n", key);
	static bool imshowFlag = 0;
	if (!imshowFlag) {
		imshowFlag = 1;
		Texture2Mat(texture);
		LoadReferenceImage(referenceImage);
		imshow("Best image", texture);
		imshow("Reference", referenceImage);
	}
	switch (key) {
	case 'q': case 'Q': case '\033':
		exit(0);
		break;

	case 'e':
		Step(cameraParameterX, cameraStep);
		break;
	case 'r':
		Step(cameraParameterX, -cameraStep);
		break;

	case 'd':
		Step(cameraParameterY, cameraStep);
		break;
	case 'f':
		Step(cameraParameterY, -cameraStep);
		break;

	case 'c':
		Step(cameraParameterZ, cameraStep);
		break;
	case 'v':
		Step(cameraParameterZ, -cameraStep);
		break;

	case 'a':
		Step(scale, scaleStep);
		break;
	case 's':
		Step(scale, -scaleStep);
		break;
	case 'z':
	//	vector<thread> t;
		for (int i = 0; i < 2; ++i)
			GetDiff();
		break;
	case 'x':
		ll diff = 1e18;
		int xx, yy, zz;
		float sc;
		CheckAllTransformation(texture, referenceImage, diff, xx, yy, zz, sc);
		printf("Result\n");
		printf("Diff: %lld, (%d %d %d) scale: %f\n", diff, xx, yy, zz, sc);
	}
	//printf("%d %d %d %f\n", cameraParameterX, cameraParameterY, cameraParameterZ, scale);
	glutPostRedisplay();
}

void MyDisplay() {
	/*
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	glPushMatrix();
	glTranslatef(0.0, 0.0, 0.0);
	glScalef(scale, scale, scale);
	glCallList(obj);
	glPopMatrix();

	glutSwapBuffers(); //swap the buffers
	*/

	glPushMatrix();
	glTranslatef(0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);

	glScalef(scale, scale, scale);
	gluLookAt(0.0, 0.0, 0.0, cameraParameterX, cameraParameterY, cameraParameterZ, 0, 1.0, 0.0);
	glCallList(obj);

	glPopMatrix();
	glFlush();

}

void MyReshape(int w, int h) {
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
}

int main(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(width, height);
	glutInitWindowPosition(0, 0);
	glutCreateWindow("Current Image");
	glClearColor(1.0, 1.0, 1.0, 1.0);
	LoadObj();
	//InitLight();
	glutDisplayFunc(MyDisplay);
	glutKeyboardFunc(MyKeyboard);
	glutReshapeFunc(MyReshape);
	glutMainLoop();
	return 0;
}