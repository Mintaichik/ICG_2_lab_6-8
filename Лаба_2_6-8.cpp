#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "math_3d.h"

GLuint VBO; // хранит указатель на буфер вершин
GLuint gWorldLocation; //указатель для доступа к всемирной матрице, представленной в виде uniform-переменной внутри шейдера.

//вершинный шейдер
//объявляет, что вершина содержит атрибут, являющийся вектором из 3 элементов типа floats, который будет иметь имя 'Position' внутри шейдера.
// uniform-переменная типа матрицы 4x4 .
//умножаем матрицу на вектор и результат передаем в gl_Position.
static const char* pVS = "                                                          \n\
#version 330                                                                        \n\
                                                                                    \n\
layout (location = 0) in vec3 Position;                                             \n\
                                                                                    \n\
uniform mat4 gWorld;                                                                \n\
                                                                                    \n\
void main()                                                                         \n\
{                                                                                   \n\
    gl_Position = gWorld * vec4(Position, 1.0);                                     \n\
}";

//пиксельный шейдер
//работа фрагментного шейдера - это определение цвета пикселя (здесь красный)
static const char* pFS = "                                                          \n\
#version 330                                                                        \n\
                                                                                    \n\
out vec4 FragColor;                                                                 \n\
                                                                                    \n\
void main()                                                                         \n\
{                                                                                   \n\
    FragColor = vec4(1.0, 0.0, 0.0, 1.0);                                           \n\
}";

static void RenderSceneCB()
{
	glClear(GL_COLOR_BUFFER_BIT);  //очистка буфера кадра

	//используем статическую переменную типа float, которую мы будем по-немного увеличивать каждый вызов функции рендера
	static float Scale = 0.0f;
	Scale += 0.001f;
 
	//Мы устанавливаем v2 и v3 в 0, поэтому у объекта координаты Y и Z не будут изменяться, и мы записываем в v1 значения синуса.
	//Это будет изменять координату X на значение, плавно переходящее от -1 и до 1
	Matrix4f World;

	World.m[0][0] = sinf(Scale); World.m[0][1] = 0.0f; World.m[0][2] = 0.0f;        World.m[0][3] = 0.0f;
	World.m[1][0] = 0.0f; World.m[1][1] = cosf(Scale); World.m[1][2] = 0.0f;        World.m[1][3] = 0.0f;
	World.m[2][0] = 0.0f; ; World.m[2][1] = 0.0f; ; World.m[2][2] = sinf(Scale); World.m[2][3] = 0.0f;
	World.m[3][0] = 0.0f; ; World.m[3][1] = 0.0f; ; World.m[3][2] = 0.0f;        World.m[3][3] = 1.0f;

	//загрузим матрицу в шейдер
	/*Первый параметр - это адрес uniform - переменной(находится после компиляции шейдера используя glGetUniformLocation()).
	Второй параметр - это количество матриц, значения которых мы обновляем.
	третий параметр в glUniformMatrix4fv() - это GL_TRUE, потому что мы поставляем матрицу упорядоченную по строкам.
	Четвертый параметр - это просто указатель на первый элемент матрицы.*/
	glUniformMatrix4fv(gWorldLocation, 1, GL_TRUE, &World.m[0][0]);

	glEnableVertexAttribArray(0);  //включает индексацию атрибутов вершины
	glBindBuffer(GL_ARRAY_BUFFER, VBO);   //буфер будет хранить массив вершин
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);  //Этот вызов говорит конвейеру как воспринимать данные внутри буфера

	glDrawArrays(GL_TRIANGLES, 0, 3);   //функция для отрисовки

	glDisableVertexAttribArray(0);  //отключает атрибут вершины

	glutSwapBuffers();  //меняет фоновой буфер и буфер кадра местами
}


static void InitializeGlutCallbacks()
{
	glutDisplayFunc(RenderSceneCB);  //начинаем работу в оконной системе
	glutIdleFunc(RenderSceneCB);  //указываем функцию в качестве ленивой (если нет изменений рендер производится только один раз)
}

static void CreateVertexBuffer()
{
	//создаем массив, содержащий 3 вершины
	Vector3f Vertices[3];
	Vertices[0] = Vector3f(-1.0f, -1.0f, 0.0f);
	Vertices[1] = Vector3f(1.0f, -1.0f, 0.0f);
	Vertices[2] = Vector3f(0.0f, 1.0f, 0.0f);

	glGenBuffers(1, &VBO);  //функция для генерации объектов переменных типов
	glBindBuffer(GL_ARRAY_BUFFER, VBO);  //буфер будет хранить массив вершин

	//принимает название цели размер данных в байтах, адрес массива вершин, и флаг, который обозначает использование паттернов для этих данных.
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);
}

static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	GLuint ShaderObj = glCreateShader(ShaderType);  //создаем шейдеры и проверяем на наличие ошибки

	if (ShaderObj == 0) {
		fprintf(stderr, "Error creating shader type %d\n", ShaderType);
		exit(0);
	}


	//Функция glShaderSource принимает тип шейдера как параметр и предоставляет гибкость в плане указания исходного кода шейдера
	//Для простоты мы используем единый массив символов для всего шейдера и мы используем только 1 слот для обоих указателей на исходник шейдера, а так же его длину. 
	//Второй параметр - это размерность обоих массивов (в нашем случае это 1).
	const GLchar* p[1];
	p[0] = pShaderText;
	GLint Lengths[1];
	Lengths[0] = strlen(pShaderText);
	glShaderSource(ShaderObj, 1, p, Lengths);

	glCompileShader(ShaderObj); //компилируем шейдер

	//Часть кода ниже получает статус компиляции и отображает все ошибки, обнаруженные компилятором.
	GLint success;
	glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar InfoLog[1024];
		glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
		fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
		exit(1);
	}

	glAttachShader(ShaderProgram, ShaderObj); //присоединяем скомпилированный объект шейдера к объекту программы
}

static void CompileShaders()
{
	GLuint ShaderProgram = glCreateProgram();  //создание программного объекта и проверяем на наличие ошибки

	if (ShaderProgram == 0) {
		fprintf(stderr, "Error creating shader program\n");
		exit(1);
	}

	//добавляем вершинный и пиксельный шейдеры
	AddShader(ShaderProgram, pVS, GL_VERTEX_SHADER);  
	AddShader(ShaderProgram, pFS, GL_FRAGMENT_SHADER);

	//получает статус компиляции и отображает все ошибки, обнаруженные компилятором.
	GLint Success = 0;
	GLchar ErrorLog[1024] = { 0 };

	glLinkProgram(ShaderProgram);  //После компиляции всех шейдеров и подсоединения их к программе мы наконец можем линковать их
	//проверяем программные ошибки
	glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(ShaderProgram, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
		exit(1);
	}

	glValidateProgram(ShaderProgram); //проверяем программу
	glGetProgramiv(ShaderProgram, GL_VALIDATE_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(ShaderProgram, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
		exit(1);
	}

	//для использования отлинкованной программы шейдеров мы назначаем её для конвейера используя вызов ниже
	//Эта программа сохранит эффект для всех вызовов отрисовки, пока вы не замените её другой или не запретите
	//её использование напрямую функцией glUseProgram с параметром NULL.
	glUseProgram(ShaderProgram);

	//После линковки программы мы запрашиваем позицию uniform-переменной в программном объекте и проверяем на ошибки
	gWorldLocation = glGetUniformLocation(ShaderProgram, "gWorld");
	assert(gWorldLocation != 0xFFFFFFFF);
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);  //инициализируем GLUT
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);  //включаем двойную буферизацию и буфер цвета

	// задаем парметры окна, создаем его и задаем ему заголовок
	glutInitWindowSize(1024, 768); 
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Tutorial 08");

	InitializeGlutCallbacks(); //вызывает функцию для работы в оконной системе

	// Must be done after glut is initialized!
	//инициализация GLEW и проверка его на ошибки
	GLenum res = glewInit();
	if (res != GLEW_OK) {
		fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
		return 1;
	}

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);  //устанавливает цвет, который будет использован во время очистки буфера кадра

	CreateVertexBuffer(); //создает буфер вершин

	CompileShaders(); //компилирует шейдеры

	glutMainLoop();  //передает контроль GLUT'у, который начнет свой собственный цикл

	return 0;
}
