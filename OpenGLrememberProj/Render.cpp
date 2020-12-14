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

int flag1 = 0;
int flag2 = 0;
int flag_der = 1;

//обработчик нажатия кнопок клавиатуры
void keyDownEvent(OpenGL *ogl, int key)
{
	
	if (OpenGL::isKeyPressed('F'))
	{
		light.pos = camera.pos;
	}

	if (OpenGL::isKeyPressed('Z'))
	{
		if (flag1 == 0)
			flag1 = 1;
		else
			flag1 = 0;
	}

	if (OpenGL::isKeyPressed('C'))
	{
		if (flag2 == 0)
			flag2 = 1;
		else	
			flag2 = 0;
	}

	if (OpenGL::isKeyPressed('S'))
	{
		if (flag_der == 0)
			flag_der = 1;
		else
			flag_der = 0;
	}

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



ObjFile QuadHouse;

ObjFile LittleHouse;

ObjFile Zabor;

ObjFile Poezd, Relsi, Earth, Vagon;
ObjFile Tree, Tree2, Kust;
ObjFile Svetofor,Svetofor_red, Svetofor_green;
ObjFile Budka, Stancia_Roof, Stancia_Floor2, Reklama, Budka_Dver;

Texture QuadHouse_Tex, QuadHouse_SpecTex, QuadHouse_FonTex, Poezd_Tex, Earth_Tex,Earth_Normal, QuadHouse_Normal;
Texture Svetofor_Tex, LittleHouse_Tex, LittleHouse_SpecTex, LittleHouse_FonTex;
Texture Tex_Reklama;
Texture Tex_Zabor;

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
	glEnable(GL_TEXTURE0);

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
	s[1].FshaderFileName = "shaders\\Light_Color.frag"; //имя файла фрагментного шейдера
	s[1].LoadShaderFromFile(); //загружаем шейдеры из файла
	s[1].Compile(); //компилируем

	s[2].VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	s[2].FshaderFileName = "shaders\\Color.frag"; //имя файла фрагментного шейдера
	s[2].LoadShaderFromFile(); //загружаем шейдеры из файла
	s[2].Compile(); //компилируем

	s[3].VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	s[3].FshaderFileName = "shaders\\light_texture.frag"; //имя файла фрагментного шейдера
	s[3].LoadShaderFromFile(); //загружаем шейдеры из файла
	s[3].Compile(); //компилируем
	
	s[4].VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	s[4].FshaderFileName = "shaders\\light_Tex_Normal.frag"; //имя файла фрагментного шейдера
	s[4].LoadShaderFromFile(); //загружаем шейдеры из файла
	s[4].Compile(); //компилируем

	s[5].VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	s[5].FshaderFileName = "shaders\\light_texture2.frag"; //имя файла фрагментного шейдера
	s[5].LoadShaderFromFile(); //загружаем шейдеры из файла
	s[5].Compile(); //компилируем

	s[6].VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	s[6].FshaderFileName = "shaders\\light_texture3.frag"; //имя файла фрагментного шейдера
	s[6].LoadShaderFromFile(); //загружаем шейдеры из файла
	s[6].Compile(); //компилируем

	 //так как гит игнорит модели *.obj файлы, так как они совпадают по расширению с объектными файлами, 
	 // создающимися во время компиляции, я переименовал модели в *.obj_m
	
	 loadModel("models\\Poezd.obj", &Poezd);
	 Poezd_Tex.loadTextureFromFile("textures//Poezd_Tex.bmp");
	 Poezd_Tex.bindTexture();


	 loadModel("models\\QuadHouse.obj", &QuadHouse);
	 QuadHouse_Tex.loadTextureFromFile("textures//Tex_QuadHouse.bmp");
	 QuadHouse_Tex.bindTexture();
	 glEnable(GL_TEXTURE1);
	 QuadHouse_SpecTex.loadTextureFromFile("textures//SpecTex_QuadHouse.bmp");
	 QuadHouse_SpecTex.bindTexture();
	 glEnable(GL_TEXTURE2);
	 QuadHouse_FonTex.loadTextureFromFile("textures//FonTex_QuadHouse.bmp");
	 QuadHouse_FonTex.bindTexture();

	 glEnable(GL_TEXTURE0);

	 loadModel("models\\LittleHouse.obj", &LittleHouse);		//маленький дом
	 LittleHouse_Tex.loadTextureFromFile("textures//Tex_LittleHouse.bmp");
	 LittleHouse_Tex.bindTexture();
	 glEnable(GL_TEXTURE1);
	 LittleHouse_SpecTex.loadTextureFromFile("textures//LittleHouse_SpecTex.bmp");
	 LittleHouse_SpecTex.bindTexture();
	 glEnable(GL_TEXTURE2);
	 LittleHouse_FonTex.loadTextureFromFile("textures//LittleHouse_FonTex.bmp");
	 LittleHouse_FonTex.bindTexture();

	 glEnable(GL_TEXTURE0);

	 loadModel("models\\Zabor.obj", &Zabor);
	
	 loadModel("models\\relsa.obj", &Relsi);		//рельсы

	 loadModel("models\\earth.obj", &Earth);		//Земля
	 Earth_Tex.loadTextureFromFile("textures//Earth_Tex.bmp");
	 Earth_Tex.bindTexture();

	 glEnable(GL_TEXTURE1);
	 Earth_Normal.loadTextureFromFile("textures//Earth_Normal.bmp");
	 Earth_Normal.bindTexture();

	 glEnable(GL_TEXTURE0);
	
	loadModel("models\\svetofor.obj", &Svetofor);					//светофор
	loadModel("models\\svetofor_red.obj", &Svetofor_red);
	loadModel("models\\svetofor_green.obj", &Svetofor_green);
	Svetofor_Tex.loadTextureFromFile("textures//Svetofor.bmp");
	Svetofor_Tex.bindTexture();

	loadModel("models\\Budka.obj", &Budka);			//станция
	loadModel("models\\Budka_Dver.obj", &Budka_Dver);
	loadModel("models\\Stancia_Floor.obj", &Stancia_Roof);
	loadModel("models\\Stancia_Floor2.obj", &Stancia_Floor2);
	loadModel("models\\Reklama.obj", &Reklama);
	Tex_Reklama.loadTextureFromFile("textures//Reklama.bmp");
	Tex_Reklama.bindTexture();
	
	loadModel("models\\Tree.obj", &Tree);			//дерево
	loadModel("models\\Tree2.obj", &Tree2);
	loadModel("models\\kust.obj", &Kust);

	tick_n = GetTickCount();
	tick_o = tick_n;

	rec.setSize(300, 100);
	rec.setPosition(10, ogl->getHeight() - 100-10);
	rec.setText("F - Свет из камеры\nG - двигать свет по горизонтали\nG+ЛКМ двигать свет по вертекали\nZ - Переключение светофора №1\nC - Переключение светофора №2\nS - Вкл/выкл деревья",0,0,0);

	
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

void Change_Material(double amb[], double dif[], double spec[], double sh)
{
	GLfloat _amb[] = { amb[0], amb[1], amb[2], 1 };
	GLfloat _dif[] = { dif[0], dif[1] , dif[2],1 };
	GLfloat _spec[] = { spec[0], spec[1], spec[2],1 };
	GLfloat _sh = 0.4;

	//фоновая
	glMaterialfv(GL_FRONT, GL_AMBIENT, _amb);
	//дифузная
	glMaterialfv(GL_FRONT, GL_DIFFUSE, _dif);
	//зеркальная
	glMaterialfv(GL_FRONT, GL_SPECULAR, _spec);
	//размер блика
	glMaterialf(GL_FRONT, GL_SHININESS, _sh);
}

void Relsi_path()
{

	double P1[] = { 0, -10, 0.05 };
	double P2[] = { 3,-10, 0.05 };
	double P3[] = { 8, -10, 0.05 };
	double P4[] = { 25, -10, 0.05 };
	double P5[] = { 25, -5, 0.05 };
	double P6[] = { 25, 10, 0.05 };
	double P7[] = { 25, 40, 0.05 };
	double P8[] = { 20, 40, 0.05 };
	double P9[] = { 5, 40, 0.05 };
	double P10[] = { 0, 40, 0.05 };
	double P11[] = { 0, 30, 0.05 };
	double P12[] = { 0, 20.5, 0.05 };
	double P13[] = { 0, 20, 0.05 };

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
		Shader::DontUseShaders();
		Relsi.DrawObj();
		glPopMatrix();
		
	}
}
void Relsi_path2()
{

	double P1[] = { 0, 20, 0.05 };
	double P2[] = { 0, 19.5, 0.05 };
	double P3[] = { 1, 15, 0.05 };
	double P4[] = { 0, 0, 0.05 };
	double P5[] = { -9, 0, 0.05 };
	double P6[] = { -25, 20, 0.05 };
	double P7[] = { -25, 15, 0.05 };
	double P8[] = { -25, 0, 0.05 };
	double P9[] = { -25, -20, 0.05 };
	double P10[] = { -15, -9.5, 0.05 };
	double P11[] = { -5, -9.5, 0.05 };
	double P12[] = { -2, -10, 0.05 };
	double P13[] = { 0, -10, 0.05 };

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
		Shader::DontUseShaders();
		Relsi.DrawObj();
		glPopMatrix();

	}
}

int flag_animacia = 0;
int location = 0 ;

double amb[] = { 0.05, 0.05, 0.05,1 };
double dif[] = { 0.5, 0.5 , 0.5,1 };
double spec[] = { 0.7, 0.7, 0.7,1 };
double sh = 0.4;

//статический поезд
void DrawningPoezd(double x, double y, double z, double angle)
{
	glPushMatrix();
	glTranslated(x, y, z);
	glRotated(angle, 0, 0, 1);

	glEnable(GL_TEXTURE_2D);
	Poezd_Tex.bindTexture();
	Poezd.DrawObj();
	glDisable(GL_TEXTURE_2D);

	glPopMatrix();
}
//первая анимация движения поезда
void Anim1_Poezd()
{
	t_max[0] += Time / 15; //t_max становится = 1 за 30 секунд

	if (t_max[0] > 1)
	{
		if (flag2 == 1)
		{
			DrawningPoezd(0, 20, 0, 270);
			return;
		}
		flag_animacia = 1;
		t_max[0] = 0; //после обнуляется
	}
	double P1[] = { 0, -10, 0 };
	double P2[] = { 3,-10, 0 };
	double P3[] = { 8, -10, 0 };
	double P4[] = { 25, -10, 0 };
	double P5[] = { 25, -5, 0 };
	double P6[] = { 25, 10, 0 };
	double P7[] = { 25, 40, 0 };
	double P8[] = { 20, 40, 0 };
	double P9[] = { 5, 40, 0 };
	double P10[] = { 0, 40, 0 };
	double P11[] = { 0, 30, 0 };
	double P12[] = { 0, 20.5, 0 };
	double P13[] = { 0, 20, 0 };


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


	glEnable(GL_TEXTURE_2D);
	Poezd_Tex.bindTexture();
	Poezd.DrawObj();
	glDisable(GL_TEXTURE_2D);
	glPopMatrix();
}
//вторая анимация движения поезда
void Anim2_Poezd()
{
	t_max[1] += Time / 15; //t_max становится = 1 за 30 секунд

	if (t_max[1] > 1)
	{
		if (flag1 == 1)
		{
			DrawningPoezd(0, -10, 0, 0);
			return;
		}
		flag_animacia = 0;
		t_max[1] = 0; //после обнуляется
	}

	double P1[] = { 0, 20, 0 };
	double P2[] = { 0, 19.5, 0 };
	double P3[] = { 1, 15, 0 };
	double P4[] = { 0, 0, 0 };
	double P5[] = { -9, 0, 0 };
	double P6[] = { -25, 20, 0 };
	double P7[] = { -25, 15, 0 };
	double P8[] = { -25, 0, 0 };
	double P9[] = { -25, -20, 0 };
	double P10[] = { -15, -9.5, 0 };
	double P11[] = { -5, -9.5, 0 };
	double P12[] = { -2, -10, 0 };
	double P13[] = { 0, -10, 0 };

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

	glEnable(GL_TEXTURE_2D);
	Poezd_Tex.bindTexture();
	Poezd.DrawObj();
	glDisable(GL_TEXTURE_2D);
	glPopMatrix();
}
//рисуем дерево
void DrawninTree1()
{
	double coords[21][3] = {
		{ -0.263,-0.167, 1.915 },
		{ -0.208,-0.398, 1.915 },
		{ 0.067,0.292, 1.674},
		{ 0.341 ,-0.072, 1.731},
		{ 0.253 ,0.092, 2.034},
		{ -0.313 ,0.545, 2.08 },
		{ -0.327 ,0.702, 2.10 },
		{ -0.51 ,0.571, 2.202 },
		{ 0.0311 ,0.410, 2.244 },
		 { 0.0963 ,0.526, 2.296 },
		 { -0.373 ,-0.061, 2.289 },
		 { 0.070 ,-0.541, 2.452 },
		 { 0.070 ,-0.716, 2.518 },
		 { 0.071 ,-0.279, 2.694 },
		 { -0.196 ,-0.260, 2.667 },
		 { -0.443 ,-0.038, 2.582 },
		 { -0.501 ,-0.033, 2.457 },
		 { -0.086 ,0.088, 2.762 },
		 { -0.2027 ,0.219, 2.689 },
		 { -0.2815 ,0.184, 2.558 },
		 { -0.3406 ,0.191, 2.626 },
	};

	glPushMatrix();

	double amb[] = { 0, 0, 0 ,1 };
	double dif[] = { 0.1, 0.35 , 0.1,1 };
	double spec[] = { 0.45, 0.55, 0.45,1 };
	double sh = 0.25;
	Change_Material(amb, dif, spec, sh);

	for (int i = 0; i <= 20; i++)
	{
			glPushMatrix();
			glTranslated(coords[i][0], coords[i][1], coords[i][2]);
			glRotated(180, 0, 0, 1);
			Kust.DrawObj();
			glPopMatrix();
	}

	double amb2[] = { 0, 0, 0, 1 };
	double dif2[] = { 0.5, 0.5 , 0,1 };
	double spec2[] = { 0.8, 0.27, 0.07,1 };
	double sh2 = 0.25;
	Change_Material(amb2, dif2, spec2, sh2);

	Tree.DrawObj();


	glPopMatrix();
}
//рисуем другое дерево
void DrawninTree2()
{
	double coords[21][3] = {
		{ 0.295,0.335, 1.448 },
		{ 0.298,0.395, 1.658 },
		{ 0.359,0.377, 1.759},
		{ 0.307 ,0.147, 1.672},
		{ 0.257 ,0.140, 1.795},
		{ 0.377 ,0.112, 1.773 },
		{ -0.448 ,0.2819, 1.789 },
		{ -0.303 ,0.352, 1.802 }
	};

	glPushMatrix();

	double amb[] = { 0, 0, 0 ,1 };
	double dif[] = { 0.1, 0.35 , 0.1,1 };
	double spec[] = { 0.45, 0.55, 0.45,1 };
	double sh = 0.25;
	Change_Material(amb, dif, spec, sh);

	double x = 0.05;
	double y = 0;
	for (double z = 0.3; z <= 1.75; z+=0.125)
	{	
		x += 0.025;
		y += 0.011;
		glPushMatrix();
		glTranslated(x,y,z);
		glRotated(180, 0, 0, 1);
		Kust.DrawObj();
		glPopMatrix();
	}
	x = -0.05;
	y = 0;
	for (double z = 0.3; z <= 1.75; z += 0.125)
	{
		x -= 0.03;
		y += 0.011;
		glPushMatrix();
		glTranslated(x, y, z);
		glRotated(180, 0, 0, 1);
		Kust.DrawObj();
		glPopMatrix();
	}

	x = 0;
	y = -0.03;
	for (double z = 0.3; z <= 1.45; z += 0.125)
	{
		y -= 0.028;
		glPushMatrix();
		glTranslated(x, y, z);
		glRotated(180, 0, 0, 1);
		Kust.DrawObj();
		glPopMatrix();
	}
	for (int i = 0; i <= 7; i++)
	{
		glPushMatrix();
		glTranslated(coords[i][0], coords[i][1], coords[i][2]);
		glRotated(180, 0, 0, 1);
		Kust.DrawObj();
		glPopMatrix();
	}

	
	double amb2[] = { 0, 0, 0, 1 };
	double dif2[] = { 0.5, 0.5 , 0,1 };
	double spec2[] = { 0.8, 0.27, 0.07,1 };
	double sh2 = 0.25;
	Change_Material(amb2, dif2, spec2, sh2);

	Tree2.DrawObj();

	glPopMatrix();
}
//рисуем маленький домик
void DrawningLittleHouse(double x, double y, double z,double angle)
{
	glEnable(GL_TEXTURE_2D);
	
	//рисует дом с забором
	glPushMatrix();

	glTranslated(x, y, z);
	glRotated(angle, 0, 0, 1);

	s[6].UseShader();
	location = glGetUniformLocationARB(s[6].program, "light_pos");
	glUniform3fARB(location, light.pos.X(), light.pos.Y(), light.pos.Z());

	location = glGetUniformLocationARB(s[6].program, "Ia");
	glUniform3fARB(location, 1, 1, 1);
	location = glGetUniformLocationARB(s[6].program, "ma");
	glUniform3fARB(location, 1, 1, 1);

	location = glGetUniformLocationARB(s[6].program, "Id");
	glUniform3fARB(location, 1, 1, 1);
	location = glGetUniformLocationARB(s[6].program, "md");
	glUniform3fARB(location, 0.6, 0.6, 0.6);

	location = glGetUniformLocationARB(s[6].program, "Is");
	glUniform3fARB(location, 1, 0.6, 0.01);
	location = glGetUniformLocationARB(s[6].program, "ms");
	glUniform4fARB(location, 1, 0.6, 0.01, 25.6);

	location = glGetUniformLocationARB(s[6].program, "camera");
	glUniform3fARB(location, camera.pos.X(), camera.pos.Y(), camera.pos.Z());

	location = glGetUniformLocationARB(s[6].program, "Translate");
	glUniform3fARB(location, x, y, z);

	glActiveTexture(GL_TEXTURE2);
	location = glGetUniformLocationARB(s[6].program, "iTexture2");
	glUniform1iARB(location, 2);
	LittleHouse_FonTex.bindTexture();

	glActiveTexture(GL_TEXTURE1);
	location = glGetUniformLocationARB(s[6].program, "iTexture1");
	glUniform1iARB(location, 1);
	LittleHouse_SpecTex.bindTexture();

	glActiveTexture(GL_TEXTURE0);
	location = glGetUniformLocationARB(s[6].program, "iTexture0");
	glUniform1iARB(location, 0);
	LittleHouse_Tex.bindTexture();

	LittleHouse.DrawObj();

	glDisable(GL_TEXTURE_2D);
	Shader::DontUseShaders();

	amb[0] = 0.15; amb[1] = 0.08; amb[2] = 0.01;
	dif[0] = 0.5; dif[1] = 0.3; dif[2] = 0.05;
	spec[0] = 0.2; spec[1] = 0.2; spec[2] = 0.2;
	sh = 0.4;
	Change_Material(amb,dif,spec,sh);

	Zabor.DrawObj();
;
	glPopMatrix();
}
//рисуем квадратный домик
void DrawningQuadHouse(double x, double y, double z, double angle)
{
	glEnable(GL_TEXTURE_2D);
	glPushMatrix();

	s[6].UseShader();
	location = glGetUniformLocationARB(s[6].program, "light_pos");
	glUniform3fARB(location, light.pos.X(), light.pos.Y(), light.pos.Z());

	location = glGetUniformLocationARB(s[6].program, "Ia");
	glUniform3fARB(location, 1, 1, 1);
	location = glGetUniformLocationARB(s[6].program, "ma");
	glUniform3fARB(location, 1, 1, 1);

	location = glGetUniformLocationARB(s[6].program, "Id");
	glUniform3fARB(location, 1, 1, 1);
	location = glGetUniformLocationARB(s[6].program, "md");
	glUniform3fARB(location, 0.6, 0.6, 0.6);

	location = glGetUniformLocationARB(s[6].program, "Is");
	glUniform3fARB(location, 1, 1, 1);
	location = glGetUniformLocationARB(s[6].program, "ms");
	glUniform4fARB(location, 1, 1, 1, 25.6);

	location = glGetUniformLocationARB(s[6].program, "camera");
	glUniform3fARB(location, camera.pos.X(), camera.pos.Y(), camera.pos.Z());

	location = glGetUniformLocationARB(s[6].program, "Translate");
	glUniform3fARB(location, x, y, z);

	glActiveTexture(GL_TEXTURE2);
	location = glGetUniformLocationARB(s[6].program, "iTexture2");
	glUniform1iARB(location, 2);
	QuadHouse_FonTex.bindTexture();

	glActiveTexture(GL_TEXTURE1);
	location = glGetUniformLocationARB(s[6].program, "iTexture1");
	glUniform1iARB(location, 1);
	QuadHouse_SpecTex.bindTexture();

	glActiveTexture(GL_TEXTURE0);
	location = glGetUniformLocationARB(s[6].program, "iTexture0");
	glUniform1iARB(location, 0);
	QuadHouse_Tex.bindTexture();

	glTranslated(x, y, z);
	glRotated(angle, 0, 0, 1);


	QuadHouse.DrawObj();
	glDisable(GL_TEXTURE_2D);
	glPopMatrix();

	Shader::DontUseShaders();
}
//рисуем первый светофор
void DrawningSvetofor1(double x, double y, double z, double angle)//рисуем светофор
{
	glEnable(GL_TEXTURE_2D);
	glPushMatrix();

	Svetofor_Tex.bindTexture();

	glTranslated(x, y, z);
	glRotated(angle, 0, 0, 1);
	
	

	Svetofor.DrawObj();

	glDisable(GL_TEXTURE_2D);

	s[2].UseShader();

	if (flag1 == 0)
	{
		location = glGetUniformLocationARB(s[2].program, "Color");
		glUniform4fARB(location, 0.15, 0.01, 0.01, 1);

		Svetofor_red.DrawObj();

		location = glGetUniformLocationARB(s[2].program, "Color");
		glUniform4fARB(location, 0.1, 0.95, 0.1, 1);

		Svetofor_green.DrawObj();
	}
	else
	{
		location = glGetUniformLocationARB(s[2].program, "Color");
		glUniform4fARB(location, 0.95, 0.1, 0.1, 1);

		Svetofor_red.DrawObj();

		location = glGetUniformLocationARB(s[2].program, "Color");
		glUniform4fARB(location, 0.01, 0.15, 0.01, 1);

		Svetofor_green.DrawObj();
	}
	glPopMatrix();

	Shader::DontUseShaders();
}
//второй светофор
void DrawningSvetofor2(double x, double y, double z, double angle)//рисуем светофор
{

	glPushMatrix();
	glTranslated(x, y, z);
	glRotated(angle, 0, 0, 1);

	Svetofor_Tex.bindTexture();

	Svetofor.DrawObj();

	s[2].UseShader();

	if (flag2 == 0)
	{
		location = glGetUniformLocationARB(s[2].program, "Color");
		glUniform4fARB(location, 0.15, 0.01, 0.01, 1);

		Svetofor_red.DrawObj();

		location = glGetUniformLocationARB(s[2].program, "Color");
		glUniform4fARB(location, 0.1, 0.95, 0.1, 1);

		Svetofor_green.DrawObj();
	}
	else
	{
		location = glGetUniformLocationARB(s[2].program, "Color");
		glUniform4fARB(location, 0.95, 0.1, 0.1, 1);

		Svetofor_red.DrawObj();

		location = glGetUniformLocationARB(s[2].program, "Color");
		glUniform4fARB(location, 0.01, 0.15, 0.01, 1);

		Svetofor_green.DrawObj();
	}
	glPopMatrix();
	Shader::DontUseShaders();
}
//рисуем дерево с перемещением
void DrawningFullTree1(double x, double y, double z, double angle)
{
	glPushMatrix();
	glTranslated(x, y, z);
	glRotated(angle, 0, 0, 1);
	DrawninTree1();
	glPopMatrix();
}
//рисуем другое дерево с перемещением
void DrawningFullTree2(double x, double y, double z, double angle)
{
	glPushMatrix();
	glTranslated(x, y, z);
	glRotated(angle, 0, 0, 1);
	DrawninTree2();
	glPopMatrix();
}
//рисуем станцию
void DrawningStancia(double x, double y, double z, double angle)//рисуем станцию
{

	glPushMatrix();
	glTranslated(x, y, z);
	glRotated(angle, 0, 0, 1);

	amb[0] = 0.15; amb[1] = 0.15; amb[2] = 0.15;
	dif[0] = 0.5; dif[1] = 0.5; dif[2] = 0.5;
	spec[0] = 0.2; spec[1] = 0.2; spec[2] = 0.2;
	sh = 0.4;
	Change_Material(amb, dif, spec, sh);

	Budka.DrawObj();
	Stancia_Floor2.DrawObj();
	
	amb[0] = 0.3; amb[1] = 0.2; amb[2] = 0.05;
	dif[0] = 0.3; dif[1] = 0.2; dif[2] = 0.05;
	spec[0] = 0.75; spec[1] = 0.6; spec[2] = 0.15;
	sh = 0.8;
	Change_Material(amb, dif, spec, sh);

	Budka_Dver.DrawObj();
	Stancia_Roof.DrawObj();
	
	
	amb[0] = 0.15; amb[1] = 0.15; amb[2] = 0.15;
	dif[0] = 0.4; dif[1] = 0.4; dif[2] = 0.4;
	spec[0] = 0.2; spec[1] = 0.2; spec[2] = 0.2;
	sh = 0.8;
	
	
	Change_Material(amb, dif, spec, sh);
	
	glPushMatrix();
	glEnable(GL_TEXTURE_2D);
	Tex_Reklama.bindTexture();
	Reklama.DrawObj(); 

	glPopMatrix();

	glPopMatrix();
	glDisable(GL_TEXTURE_2D);
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
	//if (textureMode)
	//
	//	glEnable(GL_TEXTURE_2D);

	if (lightMode)
	{	
	glEnable(GL_LIGHTING);
	}
	//альфаналожение
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	//===================================
	//Прогать тут  

	//рисуем землю

	s[4].UseShader();
	location = glGetUniformLocationARB(s[4].program, "light_pos");
	glUniform3fARB(location, light.pos.X(), light.pos.Y(), light.pos.Z());

	location = glGetUniformLocationARB(s[4].program, "Ia");
	glUniform3fARB(location, 0.3, 0.3, 0.3);
	location = glGetUniformLocationARB(s[4].program, "ma");
	glUniform3fARB(location, 0.3, 0.3, 0.3);

	location = glGetUniformLocationARB(s[4].program, "Id");
	glUniform3fARB(location, 0.5, 0.5, 0.5);
	location = glGetUniformLocationARB(s[4].program, "md");
	glUniform3fARB(location, 0.5, 0.5, 0.5);

	location = glGetUniformLocationARB(s[4].program, "Is");
	glUniform3fARB(location, 0.1, 0.1, 0.1);
	location = glGetUniformLocationARB(s[4].program, "ms");
	glUniform4fARB(location, 0.1, 0.1, 0.1, 25.6);

	location = glGetUniformLocationARB(s[4].program, "camera");
	glUniform3fARB(location, camera.pos.X(), camera.pos.Y(), camera.pos.Z());

	glActiveTexture(GL_TEXTURE1);
	location = glGetUniformLocationARB(s[4].program, "Texture1");
	glUniform1iARB(location, 1);
	Earth_Normal.bindTexture();

	glActiveTexture(GL_TEXTURE0);
	location = glGetUniformLocationARB(s[4].program, "Texture0");
	glUniform1iARB(location, 0);
	Earth_Tex.bindTexture();
	glPushMatrix();
	glTranslated(0, 0, -1.15);
	Earth.DrawObj();
	Shader::DontUseShaders();
	glPopMatrix();



#pragma region Рисуем дома



	DrawningQuadHouse(4.5, -13.5, 0, 0);
	DrawningQuadHouse(-3.5, -12, 0, 0);
	DrawningQuadHouse(-6, -13.5, 0, 0);
	DrawningQuadHouse(0, 0, 0, 0);
	DrawningQuadHouse(5, 0, 0, 0);
	DrawningQuadHouse(5, 4, 0, 0);
	DrawningQuadHouse(0, 4, 0, 0);
	DrawningQuadHouse(-5, 0, 0, 0);
	DrawningQuadHouse(-10, 0, 0, 0);
	DrawningQuadHouse(-5, -3, 0, 0);
	DrawningQuadHouse(-5, 4, 0, 0);
	DrawningQuadHouse(0, -3, 0, 0);

	DrawningLittleHouse(0, -13.5, 0, 0);
	DrawningLittleHouse(18, 9, 0, 0);

	DrawningQuadHouse(-10, -3, 0, 0);
	DrawningQuadHouse(5, -3, 0, 0);
	DrawningQuadHouse(5, 20, 0, 0);
	DrawningQuadHouse(5, 24, 0, 0);
	DrawningQuadHouse(5, 28, 0, 0);
	DrawningQuadHouse(10, 20, 0, 0);
	DrawningQuadHouse(10, 24, 0, 0);
	DrawningQuadHouse(10, 28, 0, 0);
	DrawningQuadHouse(15, 26, 0, 0);
	DrawningQuadHouse(15, 22, 0, 0);

#pragma endregion

	glDisable(GL_TEXTURE_2D);

	//рисует траву - лучше не использовать
	//for (double i = -20; i < 20; i += 1)
	//{
	//	for (double j = -20; j < 20; j += 1)
	//	{
	//		glPushMatrix();
	//		glTranslated(i, j, 0);
	//		Kust.DrawObj();
	//		glPopMatrix();
	//	}
	//}

#pragma region Рисуем деревья

	if (flag_der == 1)
	{
		DrawningFullTree2(1, 1.5, 0, 0);
		DrawningFullTree2(0, 2, 0, 0);
		DrawningFullTree2(2.5, 0, 0, 0);
		DrawningFullTree2(-5, 2.2, 0, 0);
		DrawningFullTree2(-9, -9, 0, 0);
		DrawningFullTree2(-2, -8, 0, 0);
		DrawningFullTree2(1.5, -8, 0, 0);

		DrawningFullTree1(19, 14.5, 0, 0);
		DrawningFullTree1(13, 10, 0, 0);
		DrawningFullTree1(17, 5, 0, 0);
		DrawningFullTree1(13, -5, 0, 0);

		DrawningFullTree1(-13, -7, 0, 0);
		DrawningFullTree1(-15, -5.5, 0, 0);
		DrawningFullTree1(-15, 5, 0, 0);
		DrawningFullTree1(10, 2.5, 0, 0);
		DrawningFullTree1(22, 30, 0, 0);
		DrawningFullTree1(11, 3, 0, 0);
		DrawningFullTree1(8, 6, 0, 0);
		DrawningFullTree1(7, 8, 0, 0);

		DrawningFullTree1(0, 9, 0, 0);
		DrawningFullTree1(2, 10, 0, 0);
		DrawningFullTree1(1.5, 11.5, 0, 0);
		DrawningFullTree1(-5, 9, 0, 0);
		DrawningFullTree1(-6, 12, 0, 0);
		DrawningFullTree1(3, 15, 0, 0);
		DrawningFullTree1(-2, 17, 0, 0);
		DrawningFullTree1(-2, 25, 0, 0);
		DrawningFullTree1(6, 20, 0, 0);
		DrawningFullTree1(8, 19, 0, 0);
		DrawningFullTree1(9, 22, 0, 0);
		DrawningFullTree1(8, 24, 0, 0);

		DrawningFullTree2(-2, -3, 0, 0);
		DrawningFullTree2(-2, -1, 0, 0);
		DrawningFullTree2(-1.5, -2.5, 0, 0);
	}
#pragma endregion


	Shader::DontUseShaders();
	//рисуем рельсы

	amb[0] = 0.15; amb[1] = 0.15; amb[2] = 0.15;
	dif[0] = 0.4; dif[1] = 0.4; dif[2] = 0.4;
	spec[0] = 0.8; spec[1] = 0.8; spec[2] = 0.8;
	sh = 0.4;

	Change_Material(amb, dif, spec, sh);

	Relsi_path2();
	Relsi_path();

	//переключение светофоров
	if (flag_animacia == 0)
	{
		glPushMatrix();
		Anim1_Poezd();
		glPopMatrix();
	}
	if (flag_animacia == 1)
	{
		glPushMatrix();
		Anim2_Poezd();
		glPopMatrix();
	}

	//рисуем станцию
	Shader::DontUseShaders();
	DrawningStancia(0,-9,0,0);
	DrawningStancia(-1.3,20,0,90);
	
	//рисуем светофоры
	DrawningSvetofor1(3,-9.5,0,180);
	DrawningSvetofor2(-1,15.5,0,90);
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

