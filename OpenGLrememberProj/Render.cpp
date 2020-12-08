#include "Render.h"

#include <windows.h>

#include <GL\gl.h>
#include <GL\glu.h>
#include "GL\glext.h"

#include "MyOGL.h"

#include "Camera.h"
#include "Light.h"
#include "Primitives.h"

#include "MyShaders.h"

#include "ObjLoader.h"
#include "GUItextRectangle.h"

#include "Texture.h"

GuiTextRectangle rec;

bool textureMode = true;
bool lightMode = true;


//небольшой дефайн для упрощения кода
#define POP glPopMatrix()
#define PUSH glPushMatrix()


ObjFile *model;

Texture texture1;
Texture sTex;
Texture rTex;
Texture tBox;

Shader s[10];  //массивчик для десяти шейдеров
Shader frac;
Shader cassini;




//класс для настройки камеры
class CustomCamera : public Camera
{
public:
	//дистанция камеры
	double camDist;
	//углы поворота камеры
	double fi1, fi2;

	
	//значния масеры по умолчанию
	CustomCamera()
	{
		camDist = 15;
		fi1 = 1;
		fi2 = 1;
	}

	
	//считает позицию камеры, исходя из углов поворота, вызывается движком
	virtual void SetUpCamera()
	{

		lookPoint.setCoords(0, 0, 0);

		pos.setCoords(camDist*cos(fi2)*cos(fi1),
			camDist*cos(fi2)*sin(fi1),
			camDist*sin(fi2));

		if (cos(fi2) <= 0)
			normal.setCoords(0, 0, -1);
		else
			normal.setCoords(0, 0, 1);

		LookAt();
	}

	void CustomCamera::LookAt()
	{
		gluLookAt(pos.X(), pos.Y(), pos.Z(), lookPoint.X(), lookPoint.Y(), lookPoint.Z(), normal.X(), normal.Y(), normal.Z());
	}



}  camera;   //создаем объект камеры


//класс недоделан!
class WASDcamera :public CustomCamera
{
public:
		
	float camSpeed;

	WASDcamera()
	{
		camSpeed = 0.4;
		pos.setCoords(5, 5, 5);
		lookPoint.setCoords(0, 0, 0);
		normal.setCoords(0, 0, 1);
	}

	virtual void SetUpCamera()
	{

		if (OpenGL::isKeyPressed('W'))
		{
			Vector3 forward = (lookPoint - pos).normolize()*camSpeed;
			pos = pos + forward;
			lookPoint = lookPoint + forward;
			
		}
		if (OpenGL::isKeyPressed('S'))
		{
			Vector3 forward = (lookPoint - pos).normolize()*(-camSpeed);
			pos = pos + forward;
			lookPoint = lookPoint + forward;
			
		}

		LookAt();
	}

} WASDcam;


//Класс для настройки света
class CustomLight : public Light
{
public:
	CustomLight()
	{
		//начальная позиция света
		pos = Vector3(1, 1, 5);
	}

	
	//рисует сферу и линии под источником света, вызывается движком
	void  DrawLightGhismo()
	{
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
		Shader::DontUseShaders();
		bool f1 = glIsEnabled(GL_LIGHTING);
		glDisable(GL_LIGHTING);
		bool f2 = glIsEnabled(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_2D);
		bool f3 = glIsEnabled(GL_DEPTH_TEST);
		
		glDisable(GL_DEPTH_TEST);
		glColor3d(0.9, 0.8, 0);
		Sphere s;
		s.pos = pos;
		s.scale = s.scale*0.08;
		s.Show();

		if (OpenGL::isKeyPressed('G'))
		{
			glColor3d(0, 0, 0);
			//линия от источника света до окружности
				glBegin(GL_LINES);
			glVertex3d(pos.X(), pos.Y(), pos.Z());
			glVertex3d(pos.X(), pos.Y(), 0);
			glEnd();

			//рисуем окруность
			Circle c;
			c.pos.setCoords(pos.X(), pos.Y(), 0);
			c.scale = c.scale*1.5;
			c.Show();
		}
		/*
		if (f1)
			glEnable(GL_LIGHTING);
		if (f2)
			glEnable(GL_TEXTURE_2D);
		if (f3)
			glEnable(GL_DEPTH_TEST);
			*/
	}

	void SetUpLight()
	{
		GLfloat amb[] = { 0.2, 0.2, 0.2, 0 };
		GLfloat dif[] = { 1.0, 1.0, 1.0, 0 };
		GLfloat spec[] = { .7, .7, .7, 0 };
		GLfloat position[] = { pos.X(), pos.Y(), pos.Z(), 1. };

		// параметры источника света
		glLightfv(GL_LIGHT0, GL_POSITION, position);
		// характеристики излучаемого света
		// фоновое освещение (рассеянный свет)
		glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
		// диффузная составляющая света
		glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
		// зеркально отражаемая составляющая света
		glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

		glEnable(GL_LIGHT0);
	}



} light;  //создаем источник света



//старые координаты мыши
int mouseX = 0, mouseY = 0;




float offsetX = 0, offsetY = 0;
float zoom=1;
float Time = 0;
int tick_o = 0;
int tick_n = 0;

//обработчик движения мыши
void mouseEvent(OpenGL *ogl, int mX, int mY)
{
	int dx = mouseX - mX;
	int dy = mouseY - mY;
	mouseX = mX;
	mouseY = mY;

	//меняем углы камеры при нажатой левой кнопке мыши
	if (OpenGL::isKeyPressed(VK_RBUTTON))
	{
		camera.fi1 += 0.01*dx;
		camera.fi2 += -0.01*dy;
	}


	if (OpenGL::isKeyPressed(VK_LBUTTON))
	{
		offsetX -= 1.0*dx/ogl->getWidth()/zoom;
		offsetY += 1.0*dy/ogl->getHeight()/zoom;
	}


	
	//двигаем свет по плоскости, в точку где мышь
	if (OpenGL::isKeyPressed('G') && !OpenGL::isKeyPressed(VK_LBUTTON))
	{
		LPPOINT POINT = new tagPOINT();
		GetCursorPos(POINT);
		ScreenToClient(ogl->getHwnd(), POINT);
		POINT->y = ogl->getHeight() - POINT->y;

		Ray r = camera.getLookRay(POINT->x, POINT->y,60,ogl->aspect);

		double z = light.pos.Z();

		double k = 0, x = 0, y = 0;
		if (r.direction.Z() == 0)
			k = 0;
		else
			k = (z - r.origin.Z()) / r.direction.Z();

		x = k*r.direction.X() + r.origin.X();
		y = k*r.direction.Y() + r.origin.Y();

		light.pos = Vector3(x, y, z);
	}

	if (OpenGL::isKeyPressed('G') && OpenGL::isKeyPressed(VK_LBUTTON))
	{
		light.pos = light.pos + Vector3(0, 0, 0.02*dy);
	}

	
}

//обработчик вращения колеса  мыши
void mouseWheelEvent(OpenGL *ogl, int delta)
{


	float _tmpZ = delta*0.003;
	if (ogl->isKeyPressed('Z'))
		_tmpZ *= 10;
	zoom += 0.2*zoom*_tmpZ;


	if (delta < 0 && camera.camDist <= 1)
		return;
	if (delta > 0 && camera.camDist >= 100)
		return;

	camera.camDist += 0.01*delta;
}

//обработчик нажатия кнопок клавиатуры
void keyDownEvent(OpenGL *ogl, int key)
{
	if (key == 'L')
	{
		lightMode = !lightMode;
	}

	if (key == 'T')
	{
		textureMode = !textureMode;
	}	   

	if (key == 'R')
	{
		camera.fi1 = 1;
		camera.fi2 = 1;
		camera.camDist = 15;

		light.pos = Vector3(1, 1, 3);
	}

	if (key == 'F')
	{
		light.pos = camera.pos;
	}

	if (key == 'S')
	{
		frac.LoadShaderFromFile();
		frac.Compile();

		s[0].LoadShaderFromFile();
		s[0].Compile();

		cassini.LoadShaderFromFile();
		cassini.Compile();
	}

	if (key == 'Q')
		Time = 0;
}

void keyUpEvent(OpenGL *ogl, int key)
{

}


void DrawQuad()
{
	double A[] = { 0,0 };
	double B[] = { 1,0 };
	double C[] = { 1,1 };
	double D[] = { 0,1 };
	glBegin(GL_QUADS);
	glColor3d(.5, 0, 0);
	glNormal3d(0, 0, 1);
	glTexCoord2d(0, 0);
	glVertex2dv(A);
	glTexCoord2d(1, 0);
	glVertex2dv(B);
	glTexCoord2d(1, 1);
	glVertex2dv(C);
	glTexCoord2d(0, 1);
	glVertex2dv(D);
	glEnd();
}


ObjFile Poezd, Relsi, Earth, Vagon, monkey, Tree,Kust,QuadHouse;

Texture Poezdtex;

//выполняется перед первым рендером
void initRender(OpenGL *ogl)
{

	//настройка текстур

	//4 байта на хранение пикселя
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	//настройка режима наложения текстур
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	//включаем текстуры
	glEnable(GL_TEXTURE_2D);
	
	


	//камеру и свет привязываем к "движку"
	ogl->mainCamera = &camera;\
	//ogl->mainCamera = &WASDcam;
	
	ogl->mainLight = &light;
	// нормализация нормалей : их длины будет равна 1
	glEnable(GL_NORMALIZE);

	// устранение ступенчатости для линий
	glEnable(GL_LINE_SMOOTH); 


	//   задать параметры освещения
	//  параметр GL_LIGHT_MODEL_TWO_SIDE - 
	//                0 -  лицевые и изнаночные рисуются одинаково(по умолчанию), 
	//                1 - лицевые и изнаночные обрабатываются разными режимами       
	//                соответственно лицевым и изнаночным свойствам материалов.    
	//  параметр GL_LIGHT_MODEL_AMBIENT - задать фоновое освещение, 
	//                не зависящее от сточников
	// по умолчанию (0.2, 0.2, 0.2, 1.0)

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);

	/*
	//texture1.loadTextureFromFile("textures\\texture.bmp");   загрузка текстуры из файла
	*/


	frac.VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	frac.FshaderFileName = "shaders\\frac.frag"; //имя файла фрагментного шейдера
	frac.LoadShaderFromFile(); //загружаем шейдеры из файла
	frac.Compile(); //компилируем

	cassini.VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	cassini.FshaderFileName = "shaders\\cassini.frag"; //имя файла фрагментного шейдера
	cassini.LoadShaderFromFile(); //загружаем шейдеры из файла
	cassini.Compile(); //компилируем
	

	s[0].VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	s[0].FshaderFileName = "shaders\\light.frag"; //имя файла фрагментного шейдера
	s[0].LoadShaderFromFile(); //загружаем шейдеры из файла
	s[0].Compile(); //компилируем

	s[1].VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	s[1].FshaderFileName = "shaders\\textureShader.frag"; //имя файла фрагментного шейдера
	s[1].LoadShaderFromFile(); //загружаем шейдеры из файла
	s[1].Compile(); //компилируем

	s[2].VshaderFileName = "shaders\\v.vert";
	s[2].FshaderFileName = "shaders\\TestFrag.frag";
	s[2].LoadShaderFromFile(); //загружаем шейдеры из файла
	s[2].Compile(); //компилируем


	 //так как гит игнорит модели *.obj файлы, так как они совпадают по расширению с объектными файлами, 
	 // создающимися во время компиляции, я переименовал модели в *.obj_m
	loadModel("models\\paravoz.obj", &Poezd);
	Poezdtex.loadTextureFromFile("textures//poezdtext.bmp");
	Poezdtex.bindTexture();


	loadModel("models\\relsa.obj", &Relsi);
	loadModel("models\\earth.obj", &Earth);
	loadModel("models\\vagon.obj", &Vagon);
	//loadModel("models\\monkey.obj_m", &monkey);
	//loadModel("models\\Tree2.obj", &Tree);
	//loadModel("models\\kust.obj", &Kust);
	//loadModel("models\\QuadHouse.obj", &QuadHouse);


	//glActiveTexture(GL_TEXTURE0);
	//loadModel("models\\monkey.obj_m", &monkey);
	//monkeyTex.loadTextureFromFile("textures//tex.bmp");
	//monkeyTex.bindTexture();


	tick_n = GetTickCount();
	tick_o = tick_n;

	rec.setSize(300, 100);
	rec.setPosition(10, ogl->getHeight() - 100-10);
	rec.setText("T - вкл/выкл текстур\nL - вкл/выкл освещение\nF - Свет из камеры\nG - двигать свет по горизонтали\nG+ЛКМ двигать свет по вертекали",0,0,0);

	
}

double f(double P[], double t)// формула кривой Безье
{
	

	return (P[0] * pow((1 - t), 12)) +
		(P[1] * pow((1 - t), 11) * pow(t, 1) * 12) +
		(P[2] * pow((1 - t), 10) * pow(t, 2) * 11 * 6) +
		(P[3] * pow((1 - t), 9) * pow(t, 3) * 11 * 5 * 4) +
		(P[4] * pow((1 - t), 8) * pow(t, 4) * 11 * 5 * 9) +
		(P[5] * pow((1 - t), 7) * pow(t, 5) * 12 * 11 * 6) +
		(P[6] * pow((1 - t), 6) * pow(t, 6) * 11 * 12 * 7) +
		(P[7] * pow((1 - t), 5) * pow(t, 7) * 12 * 11 * 6) +
		(P[8] * pow((1 - t), 4) * pow(t, 8) * 11 * 5 * 9) +
		(P[9] * pow((1 - t), 3) * pow(t, 9) * 11 * 5 * 4) +
		(P[10] * pow((1 - t), 2) * pow(t, 10) * 11 * 6) +
		(P[11] * pow((1 - t), 1) * pow(t, 11) * 12) +
		(P[12] * pow(t, 12));
	

	//return (P1 * pow((1 - t),3)) + (3 * t * pow((1 - t),2) * P2) + (3 *pow(t,2) * (1 - t) * P3) + pow(t,3) * P4; //посчитанная формула
}

double t_max[] = {0,0};

double Angle_Vectors(double B[])
{
	double A[] = { 1,0 };

	double length = sqrt(B[0] * B[0] + B[1] * B[1] + 0);

	B[0] = B[0] / length;
	B[1] = B[1] / length;
	//скалярное произведение векторов
	double scalar = A[0] * B[0] + A[1] * B[1];
	
	//модуль векторов
	double modul_A = sqrt(pow(A[0],2) + pow(A[1],2));
	double modul_B = sqrt(pow(B[0], 2) + pow(B[1], 2));

	//расчет косинуса угла между векторами
	double cos_vec = scalar / (modul_A * modul_B);

	return acos(cos_vec);
}

double P_before1[] = { 0,0,0 };
double P_before2[] = { 0,0,0 };
double vector_bef1[] = { 0,0 };	 double vector_bef2[] = { 0,0 };
double vector_new1[] = { 0,0 };	 double vector_new2[] = { 0,0 };

int flag = 0;

Vector3 Bezye(double P0[], double P1[], double P2[], double t)
{
	//	t_max += delta_time / 5; //t_max становится = 1 за 5 секунд
		//if (t_max > 1) t_max = 0; //после обнуляется

	double P[4];

	P[0] = f(P0, t);
	P[1] = f(P1, t);
	P[2] = f(P2, t);

	Vector3 D;
	D.setCoords(P[0], P[1], P[2]);
	return D;
}

void Relsi_path()
{

	double P1[] = { 0, -10, 1 };
	double P2[] = { 3,-10, 1 };
	double P3[] = { 8, -10, 1 };
	double P4[] = { 25, -10, 1 };
	double P5[] = { 25, -5, 1 };
	double P6[] = { 25, 10, 1 };
	double P7[] = { 25, 40, 1 };
	double P8[] = { 20, 40, 1 };
	double P9[] = { 5, 40, 1 };
	double P10[] = { 0, 40, 1 };
	double P11[] = { 0, 30, 1 };
	double P12[] = { 0, 20.5, 1 };
	double P13[] = { 0, 20, 1 };

	//double P1[] = { 0, -12, 0 };
	//double P2[] = { 1, -12, 0 };
	//double P3[] = { 2, -12, 0 };
	//double P4[] = { 10, -11, 0 };
	//double P5[] = { 11, 0, 0 };
	//double P6[] = { 11, 33, 0 };
	//double P7[] = { 0, 33.5, 0 };
	//double P8[] = { -11, 33, 0 };
	//double P9[] = { -19.75, 23.85, 0 };
	//double P10[] = { -20.15, 13, 0 };
	//double P11[] = { -20.1, 1.95, 0 };
	//double P12[] = { -11, -7, 0 };
	//double P13[] = { 0, -6.85, 0 };

	double P_0[] = { P1[0],P2[0],
					 P3[0],P4[0],
					 P5[0],P6[0],
					 P7[0],P8[0],
					 P9[0],P10[0],
					 P11[0],P12[0],P13[0]
	};
	double P_1[] = { P1[1],P2[1],
					 P3[1],P4[1],
					 P5[1],P6[1],
					 P7[1],P8[1],
					 P9[1],P10[1],
					 P11[1],P12[1],P13[1]
	};
	double P_2[] = { P1[2],P2[2],
					 P3[2],P4[2],
					 P5[2],P6[2],
					 P7[2],P8[2],
					 P9[2],P10[2],
					 P11[2],P12[2],P13[2]
	};

	

	for (double t = 0; t <= 1.001; t += 0.003)
	{
		Vector3 P_old = Bezye(P_0, P_1, P_2, t-0.001);

		Vector3 P = Bezye(P_0, P_1, P_2, t);
		Vector3 VecP_P_old = (P - P_old).normolize();
		
		Vector3 rotateX(VecP_P_old.X(), VecP_P_old.Y(), 0);
		rotateX = rotateX.normolize();

		Vector3 VecPrX = Vector3(1, 0, 0).vectProisvedenie(rotateX);
		double CosX = Vector3(1, 0, 0).ScalarProizv(rotateX);
		double SinAngleZ = VecPrX.Z() / abs(VecPrX.Z());
		double AngleOZ = acos(CosX) * 180 / PI * SinAngleZ;

		double AngleOY = acos(VecP_P_old.Z()) * 180 / PI - 90;

		double A[] = { -0.5,-0.5,-0.5 };
		glPushMatrix();
		glTranslated(P.X(), P.Y(), P.Z());
		glRotated(AngleOZ, 0, 0, 1);
		glRotated(AngleOY, 0, 1, 0);
		Relsi.DrawObj();
		glPopMatrix();
		
	}
}
void Relsi_path2()
{

	double P1[] = { 0, 20, 1 };
	double P2[] = { 0, 19.5, 1 };
	double P3[] = { 1, 15, 1 };
	double P4[] = { 0, 0, 1 };
	double P5[] = { -9, 0, 1 };
	double P6[] = { -25, 20, 1 };
	double P7[] = { -25, 15, 1 };
	double P8[] = { -25, 0, 1 };
	double P9[] = { -25, -20, 1 };
	double P10[] = { -15, -9.5, 1 };
	double P11[] = { -5, -9.5, 1 };
	double P12[] = { -2, -10, 1 };
	double P13[] = { 0, -10, 1 };

	double P_0[] = { P1[0],P2[0],
					 P3[0],P4[0],
					 P5[0],P6[0],
					 P7[0],P8[0],
					 P9[0],P10[0],
					 P11[0],P12[0],P13[0]
	};
	double P_1[] = { P1[1],P2[1],
					 P3[1],P4[1],
					 P5[1],P6[1],
					 P7[1],P8[1],
					 P9[1],P10[1],
					 P11[1],P12[1],P13[1]
	};
	double P_2[] = { P1[2],P2[2],
					 P3[2],P4[2],
					 P5[2],P6[2],
					 P7[2],P8[2],
					 P9[2],P10[2],
					 P11[2],P12[2],P13[2]
	};



	for (double t = 0; t <= 1.001; t += 0.003)
	{
		Vector3 P_old = Bezye(P_0, P_1, P_2, t - 0.001);

		Vector3 P = Bezye(P_0, P_1, P_2, t);
		Vector3 VecP_P_old = (P - P_old).normolize();

		Vector3 rotateX(VecP_P_old.X(), VecP_P_old.Y(), 0);
		rotateX = rotateX.normolize();

		Vector3 VecPrX = Vector3(1, 0, 0).vectProisvedenie(rotateX);
		double CosX = Vector3(1, 0, 0).ScalarProizv(rotateX);
		double SinAngleZ = VecPrX.Z() / abs(VecPrX.Z());
		double AngleOZ = acos(CosX) * 180 / PI * SinAngleZ;

		double AngleOY = acos(VecP_P_old.Z()) * 180 / PI - 90;

		double A[] = { -0.5,-0.5,-0.5 };
		glPushMatrix();
		glTranslated(P.X(), P.Y(), P.Z());
		glRotated(AngleOZ, 0, 0, 1);
		glRotated(AngleOY, 0, 1, 0);
		Relsi.DrawObj();
		glPopMatrix();

	}
}


void Anim1_Poezd()
{

	t_max[0] += Time / 30; //t_max становится = 1 за 10 секунд

	if (t_max[0] > 1) t_max[0] = 0; //после обнуляется

	double P1[] = { 0, -10, 1 };
	double P2[] = { 3,-10, 1 };
	double P3[] = { 8, -10, 1 };
	double P4[] = { 25, -10, 1 };
	double P5[] = { 25, -5, 1 };
	double P6[] = { 25, 10, 1 };
	double P7[] = { 25, 40, 1 };
	double P8[] = { 20, 40, 1 };
	double P9[] = { 5, 40, 1 };
	double P10[] = { 0, 40, 1 };
	double P11[] = { 0, 30, 1 };
	double P12[] = { 0, 20.5, 1 };
	double P13[] = { 0, 20, 1 };

	//double P1[] = { 0, -6.85, 0 };
	//double P2[] = { 11, -7, 0 };
	//double P3[] = { 20.25, 2, 0 };
	//double P4[] = { 20, 13, 0 };
	//double P5[] = { 19.5, 23.8, 0 };
	//double P6[] = { 11, 33, 0 };
	//double P7[] = { 0, 33.5, 0 };
	//double P8[] = { -11, 33, 0 };
	//double P9[] = { -19.75, 23.85, 0 };
	//double P10[] = { -20.15, 13, 0 };
	//double P11[] = { -20.1, 1.95, 0 };
	//double P12[] = { -11, -7, 0 };
	//double P13[] = { 0, -6.85, 0 };

	glBegin(GL_LINE_STRIP);
	glVertex3dv(P1); //отрисовка векторов
	glVertex3dv(P2);
	glVertex3dv(P3);
	glVertex3dv(P4);
	glVertex3dv(P5);
	glVertex3dv(P6);
	glVertex3dv(P7);
	glVertex3dv(P8);
	glVertex3dv(P9);
	glVertex3dv(P10);
	glVertex3dv(P11);
	glVertex3dv(P12);
	glVertex3dv(P13);
	glEnd();


	double P_0[] = { P1[0],P2[0],
					 P3[0],P4[0],
					 P5[0],P6[0],
					 P7[0],P8[0],
					 P9[0],P10[0],
					 P11[0],P12[0],P13[0]
	};
	double P_1[] = { P1[1],P2[1],
					 P3[1],P4[1],
					 P5[1],P6[1],
					 P7[1],P8[1],
					 P9[1],P10[1],
					 P11[1],P12[1],P13[1]
	};
	double P_2[] = { P1[2],P2[2],
					 P3[2],P4[2],
					 P5[2],P6[2],
					 P7[2],P8[2],
					 P9[2],P10[2],
					 P11[2],P12[2],P13[2]
	};

	
	//for (double t = 0; t <= t_max; t += 0.0001)
	//{
	//	
	//	glVertex3dv(P_new); //Рисуем точку P
	//}
	
	Vector3 P_old = Bezye(P_0, P_1, P_2, t_max[0] - 0.001);

	Vector3 P = Bezye(P_0, P_1, P_2, t_max[0]);
	Vector3 VecP_P_old = (P - P_old).normolize();

	Vector3 rotateX(VecP_P_old.X(), VecP_P_old.Y(), 0);
	rotateX = rotateX.normolize();

	Vector3 VecPrX = Vector3(1, 0, 0).vectProisvedenie(rotateX);
	double CosX = Vector3(1, 0, 0).ScalarProizv(rotateX);
	double SinAngleZ = VecPrX.Z() / abs(VecPrX.Z());
	double AngleOZ = acos(CosX) * 180 / PI * SinAngleZ;
	double AngleOY = acos(VecP_P_old.Z()) * 180 / PI - 90;


	glPushMatrix();

	glTranslated(P.X(), P.Y(), P.Z());
	glRotated(AngleOY, 0, 1, 0);
	glRotated(AngleOZ, 0, 0, 1);

	glPushMatrix();
	glTranslated(-1, 0.05, 0);
	
	Vagon.DrawObj();
	glPopMatrix();

	Poezd.DrawObj();
	
	glPopMatrix();

}

void Anim2_Poezd()
{

	t_max[1] += Time / 30; //t_max становится = 1 за 10 секунд

	if (t_max[1] > 1) t_max[1] = 0; //после обнуляется

	double P1[] = { 0, 20, 1 };
	double P2[] = { 0, 19.5, 1 };
	double P3[] = { 1, 15, 1 };
	double P4[] = { 0, 0, 1 };
	double P5[] = { -9, 0, 1 };
	double P6[] = { -25, 20, 1 };
	double P7[] = { -25, 15, 1 };
	double P8[] = { -25, 0, 1 };
	double P9[] = { -25, -20, 1 };
	double P10[] = { -15, -9.5, 1 };
	double P11[] = { -5, -9.5, 1 };
	double P12[] = { -2, -10, 1 };
	double P13[] = { 0, -10, 1 };


	glBegin(GL_LINE_STRIP);
	glVertex3dv(P1); //отрисовка векторов
	glVertex3dv(P2);
	glVertex3dv(P3);
	glVertex3dv(P4);
	glVertex3dv(P5);
	glVertex3dv(P6);
	glVertex3dv(P7);
	glVertex3dv(P8);
	glVertex3dv(P9);
	glVertex3dv(P10);
	glVertex3dv(P11);
	glVertex3dv(P12);
	glVertex3dv(P13);
	glEnd();


	double P_0[] = { P1[0],P2[0],
					 P3[0],P4[0],
					 P5[0],P6[0],
					 P7[0],P8[0],
					 P9[0],P10[0],
					 P11[0],P12[0],P13[0]
	};
	double P_1[] = { P1[1],P2[1],
					 P3[1],P4[1],
					 P5[1],P6[1],
					 P7[1],P8[1],
					 P9[1],P10[1],
					 P11[1],P12[1],P13[1]
	};
	double P_2[] = { P1[2],P2[2],
					 P3[2],P4[2],
					 P5[2],P6[2],
					 P7[2],P8[2],
					 P9[2],P10[2],
					 P11[2],P12[2],P13[2]
	};


	//for (double t = 0; t <= t_max; t += 0.0001)
	//{
	//	
	//	glVertex3dv(P_new); //Рисуем точку P
	//}

	Vector3 P_old = Bezye(P_0, P_1, P_2, t_max[1] - 0.001);

	Vector3 P = Bezye(P_0, P_1, P_2, t_max[1]);
	Vector3 VecP_P_old = (P - P_old).normolize();

	Vector3 rotateX(VecP_P_old.X(), VecP_P_old.Y(), 0);
	rotateX = rotateX.normolize();

	Vector3 VecPrX = Vector3(1, 0, 0).vectProisvedenie(rotateX);
	double CosX = Vector3(1, 0, 0).ScalarProizv(rotateX);
	double SinAngleZ = VecPrX.Z() / abs(VecPrX.Z());
	double AngleOZ = acos(CosX) * 180 / PI * SinAngleZ;
	double AngleOY = acos(VecP_P_old.Z()) * 180 / PI - 90;


	glPushMatrix();

	glTranslated(P.X(), P.Y(), P.Z());
	glRotated(AngleOY, 0, 1, 0);
	glRotated(AngleOZ, 0, 0, 1);

	glPushMatrix();
	glTranslated(-1, 0.05, 0);

	Vagon.DrawObj();
	glPopMatrix();

	Poezd.DrawObj();

	glPopMatrix();

}

void Render(OpenGL* ogl)
{

	tick_o = tick_n;
	tick_n = GetTickCount();
	Time = (tick_n - tick_o) / 1000.0;

	/*
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 0, 1, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	*/

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

	//glMatrixMode(GL_MODELVIEW);
	//glLoadIdentity();

	glEnable(GL_DEPTH_TEST);
	if (textureMode)
		glEnable(GL_TEXTURE_2D);

	if (lightMode)
	{	
	glEnable(GL_LIGHTING);
	}
	//альфаналожение
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//настройка материала
	GLfloat amb[] = { 0.1745, 0.01175, 0.01175,1};
	GLfloat dif[] = { 0.61424, 0.04136 , 0.04136,1};
	GLfloat spec[] = { 0.727811, 0.626959, 0.626959,1};
	GLfloat sh = 0.4;
	
	//фоновая
	glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
	//дифузная
	glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
	//зеркальная
	glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
	//размер блика
	glMaterialf(GL_FRONT, GL_SHININESS, sh);

	//===================================
	//Прогать тут  


	//
	

	s[0].UseShader();

	//передача параметров в шейдер.  Шаг один - ищем адрес uniform переменной по ее имени. 
	int location = glGetUniformLocationARB(s[0].program, "light_pos");
	//Шаг 2 - передаем ей значение
	glUniform3fARB(location, light.pos.X(), light.pos.Y(), light.pos.Z());
	
	//location = glGetUniformLocationARB(s[0].program, "light_pos2");
	////Шаг 2 - передаем ей значение
	//glUniform3fARB(location, light.pos.X()+5, light.pos.Y()+5, light.pos.Z());
	
	location = glGetUniformLocationARB(s[0].program, "Ia");
	glUniform3fARB(location, 0.2, 0.2, 0.05);
	
	location = glGetUniformLocationARB(s[0].program, "Id");
	glUniform3fARB(location, 1.0, 1.0, 1.0);
	
	location = glGetUniformLocationARB(s[0].program, "Is");
	glUniform3fARB(location, .3, .3, .3);
	
	
	location = glGetUniformLocationARB(s[0].program, "ma");
	glUniform3fARB(location, 0.2, 0.2, 0.05);
	
	location = glGetUniformLocationARB(s[0].program, "md");
	glUniform3fARB(location, 0.6, 0.65, 0.3);
	
	location = glGetUniformLocationARB(s[0].program, "ms");
	glUniform4fARB(location, 0.3, 0.3, 0.2, 25.6);


	//location = glGetUniformLocationARB(s[0].program, "Ia");
	//glUniform3fARB(location, 0.1745, 0.01175, 0.01175);
	//
	//location = glGetUniformLocationARB(s[0].program, "Id");
	//glUniform3fARB(location, 0.61424, 0.04136, 0.04136);
	//
	//location = glGetUniformLocationARB(s[0].program, "Is");
	//glUniform3fARB(location, 0.727811, 0.626959, 0.626959);
	//
	//
	//location = glGetUniformLocationARB(s[0].program, "ma");
	//glUniform3fARB(location, 0.1745, 0.01175, 0.01175);
	//
	//location = glGetUniformLocationARB(s[0].program, "md");
	//glUniform3fARB(location, 0.61424, 0.04136, 0.04136);
	//
	//location = glGetUniformLocationARB(s[0].program, "ms");
	//glUniform4fARB(location, 0.727811, 0.626959, 0.626959, 25.6);
	
	location = glGetUniformLocationARB(s[0].program, "camera");
	glUniform3fARB(location, camera.pos.X(), camera.pos.Y(), camera.pos.Z());

	Earth.DrawObj();
	

	//первый пистолет
	//objModel.DrawObj();

	//
	//Tree.DrawObj();
	
	//glPushMatrix();
	//glTranslated(-10,-5, 0);
	//Tree.DrawObj();
	//glPopMatrix();
	//
	//glPushMatrix();
	//glTranslated(10, 5, 0);
	//Tree.DrawObj();
	//glPopMatrix();

	Relsi_path2();
	Relsi_path();
	
	Anim1_Poezd();
	Anim2_Poezd();
	
	Kust.DrawObj();
	//
	QuadHouse.DrawObj();
	


	//Shader::DontUseShaders();
	//glTranslated(-5, 15, 0);
	//objModel.DrawObj();
	
	//второй, без шейдеров
	//glPushMatrix();
	//	glTranslated(-5,15,0);
	//	//glScaled(-1.0,1.0,1.0);
	//	objModel.DrawObj();
	//glPopMatrix();






	
	

	

	
}   //конец тела функции


bool gui_init = false;

//рисует интерфейс, вызывется после обычного рендера
void RenderGUI(OpenGL *ogl)
{
	
	Shader::DontUseShaders();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, ogl->getWidth(), 0, ogl->getHeight(), 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_LIGHTING);
	

	glActiveTexture(GL_TEXTURE0);
	rec.Draw();


		
	Shader::DontUseShaders(); 

}

void resizeEvent(OpenGL *ogl, int newW, int newH)
{
	rec.setPosition(10, newH - 100 - 10);
}

