#include "string"
#include <Windows.h>
#include "mropencv.h"
#include "mrgl.h"
#include "Glasses.h"
using namespace std;
const double scale = 1.5;
String cascadeName = "haarcascade_frontalface_alt2.xml";
String nestedCascadeName = "haarcascade_eye.xml";
CascadeClassifier cascade, nestedCascade;
Mat frame;
CGlasses m_glasses;
GLfloat glasses_x=0, glasses_y=0, glasses_scale=1.0f;
void detectAndDraw(Mat& img,CascadeClassifier& cascade, CascadeClassifier& nestedCascade,double scale);
void initCV()
{
	if (!cascade.load(cascadeName))
	{
		cerr << "ERROR: Could not load classifier cascade" << endl;
		return ;
	}

	if (!nestedCascade.load(nestedCascadeName))
	{
		cerr << "WARNING: Could not load classifier cascade for nested objects" << endl;
		return ;
	}
}
void initGL()
{
	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f, (GLfloat)640 / (GLfloat)480, 0.1f, 100.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	GLuint		texture[1];
	glGenTextures(1, &texture[0]);
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	GLfloat light_pos[4];
	light_pos[0] = 0;
	light_pos[1] = 0;
	light_pos[2] = -2.0;
	light_pos[3] = 0;
	glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
	glEnable(GL_LIGHT0);
	m_glasses.ReadData();
}
void DrawVideo()
{
	glTranslatef(0.0f, 0.0f, -2.0f);
	flip(frame, frame,0);
	cvtColor(frame, frame, CV_RGB2BGR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3,frame.cols, frame.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, frame.data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); 
	glVertex3f(-1.0f, -1.0f,0.0f);
	glTexCoord2f(1.0f, 0.0f); 
	glVertex3f(1.0f, -1.0f, 0.0f);
	glTexCoord2f(1.0f, 1.0f); 
	glVertex3f(1.0f, 1.0f, 0.0f);
	glTexCoord2f(0.0f, 1.0f); 
	glVertex3f(-1.0f, 1.0f, 0.0f);
	glEnd();
}
void onDraw(void* param)
{  
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glLoadIdentity();
	glPushMatrix();
//	cout << glasses_x<< "," << glasses_y << endl;
	glTranslatef((glasses_x - 320)/320.0f, (280-glasses_y)/240.0f, -3.0f+glasses_scale*0.01f);
	cout << glasses_scale<< endl;
//	glTranslatef((glasses_x - 320) / 320.0f, (280 - glasses_y) / 240.0f, -1.6f-glasses_scale/128.0);
//	glTranslatef(0.0f, 0.0f, -1.6f);
//	glRotatef(30, 0, 1, 0);
	glScalef(0.3, 0.3, 0.3);
	m_glasses.Draw();
	glPopMatrix();
	glPushMatrix();
	DrawVideo();
	glPopMatrix();
}

int main(void)
{
	string openGLWindowName = "OpenWear";
	namedWindow(openGLWindowName, WINDOW_OPENGL);
	initGL();
	initCV();
	resizeWindow(openGLWindowName, 640, 480);
	setOpenGlContext(openGLWindowName);
	setOpenGlDrawCallback(openGLWindowName, onDraw, NULL);	
	VideoCapture capture(0);
	while (1)
	{
		capture >> frame;
		if (frame.empty())
			break;
		detectAndDraw(frame.clone(), cascade, nestedCascade, scale);
		updateWindow(openGLWindowName);
		waitKey(1);
	}
	return 0;
}

void detectAndDraw(Mat& img,CascadeClassifier& cascade, CascadeClassifier& nestedCascade,double scale)
{
	int i = 0;
	double t = 0;
	vector<Rect> faces;
	const static Scalar colors[] = { CV_RGB(0, 0, 255),
		CV_RGB(0, 128, 255),
		CV_RGB(0, 255, 255),
		CV_RGB(0, 255, 0),
		CV_RGB(255, 128, 0),
		CV_RGB(255, 255, 0),
		CV_RGB(255, 0, 0),
		CV_RGB(255, 0, 255) };//用不同的颜色表示不同的人脸

	Mat gray, smallImg(cvRound(img.rows / scale), cvRound(img.cols / scale), CV_8UC1);//将图片缩小，加快检测速度
	cvtColor(img, gray, CV_BGR2GRAY);//因为用的是类haar特征，所以都是基于灰度图像的，这里要转换成灰度图像
	resize(gray, smallImg, smallImg.size(), 0, 0, INTER_LINEAR);//将尺寸缩小到1/scale,用线性插值
	equalizeHist(smallImg, smallImg);//直方图均衡
	t = (double)cvGetTickCount();//用来计算算法执行时间
	//检测人脸
	//detectMultiScale函数中smallImg表示的是要检测的输入图像为smallImg，faces表示检测到的人脸目标序列，1.1表示
	//每次图像尺寸减小的比例为1.1，2表示每一个目标至少要被检测到3次才算是真的目标(因为周围的像素和不同的窗口大
	//小都可以检测到人脸),CV_HAAR_SCALE_IMAGE表示不是缩放分类器来检测，而是缩放图像，Size(30, 30)为目标的
	//最小最大尺寸
	cascade.detectMultiScale(smallImg, faces,
		1.1, 2, 0
		//|CV_HAAR_FIND_BIGGEST_OBJECT
		//|CV_HAAR_DO_ROUGH_SEARCH
		| CV_HAAR_SCALE_IMAGE
		,
		Size(30, 30));
	t = (double)cvGetTickCount() - t;//相减为算法执行的时间
//	printf("detection time = %g ms\n", t / ((double)cvGetTickFrequency()*1000.));
	for (auto r = faces.begin(); r != faces.end(); r++, i++)
	{
		Mat smallImgROI;
		vector<Rect> nestedObjects;
		Point center;
		Scalar color = colors[i % 8];
		int radius;
		center.x = cvRound((r->x + r->width*0.5)*scale);//还原成原来的大小
		center.y = cvRound((r->y + r->height*0.5)*scale);
		radius = cvRound((r->width + r->height)*0.25*scale);
		glasses_x = center.x;
		glasses_y = center.y;
		glasses_scale = radius;
		circle(img, center, radius, color, 3, 8, 0);
		//检测人眼，在每幅人脸图上画出人眼
		if (nestedCascade.empty())
			continue;
		smallImgROI = smallImg(*r);
		//和上面的函数功能一样
		nestedCascade.detectMultiScale(smallImgROI, nestedObjects,
			1.1, 2, 0
			//|CV_HAAR_FIND_BIGGEST_OBJECT
			//|CV_HAAR_DO_ROUGH_SEARCH
			//|CV_HAAR_DO_CANNY_PRUNING
			| CV_HAAR_SCALE_IMAGE
			,
			Size(30, 30));
		for (auto nr = nestedObjects.begin(); nr != nestedObjects.end(); nr++)
		{
			center.x = cvRound((r->x + nr->x + nr->width*0.5)*scale);
			center.y = cvRound((r->y + nr->y + nr->height*0.5)*scale);
			radius = cvRound((nr->width + nr->height)*0.25*scale);
			circle(img, center, radius, color, 3, 8, 0);//将眼睛也画出来，和对应人脸的图形是一样的
		}
	}
	cv::imshow("detected eyes", img);
}