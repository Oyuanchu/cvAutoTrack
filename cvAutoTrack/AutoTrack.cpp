#include "pch.h"
#include "AutoTrack.h"

AutoTrack::AutoTrack()
{
	_detectorAllMap = new cv::Ptr<cv::xfeatures2d::SURF>;
	_detectorSomeMap = new cv::Ptr<cv::xfeatures2d::SURF>;
	_KeyPointAllMap = new std::vector<cv::KeyPoint>;
	_KeyPointSomeMap = new std::vector<cv::KeyPoint>;
	_KeyPointMiniMap = new std::vector<cv::KeyPoint>;
	_DataPointAllMap = new cv::Mat;
	_DataPointSomeMap = new cv::Mat;
	_DataPointMiniMap = new cv::Mat;
}

AutoTrack::~AutoTrack(void)
{
	delete _detectorAllMap;
	delete _detectorSomeMap;
	delete _KeyPointAllMap;
	delete _KeyPointSomeMap;
	delete _KeyPointMiniMap;
	delete _DataPointAllMap;
	delete _DataPointSomeMap;
	delete _DataPointMiniMap;
}

bool AutoTrack::init()
{
	if (!is_init_end)
	{
		cv::Ptr<cv::xfeatures2d::SURF>& detectorAllMap = *(cv::Ptr<cv::xfeatures2d::SURF>*) _detectorAllMap;
		std::vector<cv::KeyPoint>& KeyPointAllMap = *(std::vector<cv::KeyPoint>*)_KeyPointAllMap;
		cv::Mat& DataPointAllMap = *(cv::Mat*)_DataPointAllMap;

		detectorAllMap = cv::xfeatures2d::SURF::create(minHessian);
		detectorAllMap->detectAndCompute(giMatchResource.MapTemplate, cv::noArray(), KeyPointAllMap, DataPointAllMap);

		is_init_end = true;
	}
	return is_init_end;
}

bool AutoTrack::uninit()
{
	if (is_init_end)
	{
		delete _detectorAllMap;
		_detectorAllMap = nullptr;
		delete _KeyPointAllMap;
		_KeyPointAllMap = nullptr;
		delete _DataPointAllMap;
		_DataPointAllMap = nullptr;

		is_init_end = false;
	}
	return !is_init_end;
}

bool AutoTrack::GetTransform(float & x, float & y, float & a)
{
	if (!is_init_end)
	{
		error_code = 1;//未初始化
		return false;
	}
	// 判断原神窗口不存在直接返回false，不对参数做任何修改
	if (getGengshinImpactWnd())
	{
		getGengshinImpactRect();
		getGengshinImpactScreen();


		if (!giFrame.empty())
		{
			getPaimonRefMat();

			cv::Mat paimonTemplate;

			cv::resize(giMatchResource.PaimonTemplate, paimonTemplate, giPaimonRef.size());

			cv::Mat tmp;

#ifdef _DEBUG

#define Mode1

#ifdef Mode1
			giPaimonRef = giFrame(cv::Rect(0, 0, cvCeil(giFrame.cols / 20), cvCeil(giFrame.rows / 10)));

#endif // Mode1

#ifdef Mode2
			cv::Ptr<cv::xfeatures2d::SURF> detectorPaimon = cv::xfeatures2d::SURF::create(minHessian);
			std::vector<cv::KeyPoint> KeyPointPaimonTemplate, KeyPointPaimonRef;
			cv::Mat DataPointPaimonTemplate, DataPointPaimonRef;

			detectorPaimon->detectAndCompute(giPaimonRef, cv::noArray(), KeyPointPaimonRef, DataPointPaimonRef);
			detectorPaimon->detectAndCompute(paimonTemplate, cv::noArray(), KeyPointPaimonTemplate, DataPointPaimonTemplate);
			cv::Ptr<cv::DescriptorMatcher> matcherPaimonTmp = cv::DescriptorMatcher::create(cv::DescriptorMatcher::FLANNBASED);
			std::vector< std::vector<cv::DMatch> > KNN_PaimonmTmp;
			std::vector<cv::DMatch> good_matchesPaimonTmp;

			for (size_t i = 0; i < KNN_PaimonmTmp.size(); i++)
			{
				if (KNN_PaimonmTmp[i][0].distance < ratio_thresh * KNN_PaimonmTmp[i][1].distance)
				{
					good_matchesPaimonTmp.push_back(KNN_PaimonmTmp[i][0]);
				}
			}

			cv::Mat img_matchesA, imgmapA, imgminmapA;
			drawKeypoints(giPaimonRef, KeyPointPaimonRef, imgmapA, cv::Scalar::all(-1), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
			drawKeypoints(paimonTemplate, KeyPointPaimonTemplate, imgminmapA, cv::Scalar::all(-1), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
			drawMatches(paimonTemplate, KeyPointPaimonTemplate, giPaimonRef, KeyPointPaimonRef, good_matchesPaimonTmp, img_matchesA, cv::Scalar::all(-1), cv::Scalar::all(-1), std::vector<char>(), cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

			cv::imshow("asda", img_matchesA);

			if (good_matchesPaimonTmp.size() < 7)
			{
				error_code = 6;//未能匹配到派蒙
				return false;
			}
#endif // Mode2

#endif

#ifdef _DEBUG
			cv::imshow("test", giPaimonRef);
#endif

#ifdef Mode1

			cv::matchTemplate(paimonTemplate, giPaimonRef, tmp, cv::TM_CCOEFF_NORMED);

			double minVal, maxVal;
			cv::Point minLoc, maxLoc;
			cv::minMaxLoc(tmp, &minVal, &maxVal, &minLoc, &maxLoc);

#ifdef _DEBUG
			cv::imshow("test2", tmp);
			std::cout << "Paimon Match: "<< minVal<<","<<maxVal << std::endl;
#endif

			if (maxVal < 0.36 || maxVal == 1)
			{
				error_code = 6;//未能匹配到派蒙
				return false;
			}
#endif

			getMiniMapRefMat();

			cv::Mat img_scene(giMatchResource.MapTemplate);
			cv::Mat img_object(giMiniMapRef(cv::Rect(30, 30, giMiniMapRef.cols - 60, giMiniMapRef.rows - 60)));

			cv::cvtColor(img_scene, img_scene, CV_RGBA2RGB);

			if (img_object.empty())
			{
				error_code = 5;//原神小地图区域为空或者区域长宽小于60px
				return false;
			}

			isContinuity = false;

			cv::Point2f *hisP = _TransformHistory;

			cv::Point2f pos;

			if ((dis(hisP[1] - hisP[0]) + dis(hisP[2] - hisP[1])) < 2000)
			{
				if (hisP[2].x > someSizeR && hisP[2].x < img_scene.cols - someSizeR && hisP[2].y>someSizeR && hisP[2].y < img_scene.rows - someSizeR)
				{
					isContinuity = true;
					if (isContinuity)
					{
						cv::Mat someMap(img_scene(cv::Rect(cvRound(hisP[2].x - someSizeR), cvRound(hisP[2].y - someSizeR), cvRound(someSizeR * 2), cvRound(someSizeR * 2))));
						cv::Mat minMap(img_object);

						//resize(someMap, someMap, Size(), MatchMatScale, MatchMatScale, 1);
						//resize(minMap, minMap, Size(), MatchMatScale, MatchMatScale, 1);

						cv::Ptr<cv::xfeatures2d::SURF>& detectorSomeMap = *(cv::Ptr<cv::xfeatures2d::SURF>*)_detectorSomeMap;
						std::vector<cv::KeyPoint>& KeyPointSomeMap = *(std::vector<cv::KeyPoint>*)_KeyPointSomeMap;
						cv::Mat& DataPointSomeMap = *(cv::Mat*)_DataPointSomeMap;
						std::vector<cv::KeyPoint>& KeyPointMiniMap = *(std::vector<cv::KeyPoint>*)_KeyPointMiniMap;
						cv::Mat& DataPointMiniMap = *(cv::Mat*)_DataPointMiniMap;

						detectorSomeMap = cv::xfeatures2d::SURF::create(minHessian);
						detectorSomeMap->detectAndCompute(someMap, cv::noArray(), KeyPointSomeMap, DataPointSomeMap);
						detectorSomeMap->detectAndCompute(minMap, cv::noArray(), KeyPointMiniMap, DataPointMiniMap);
						cv::Ptr<cv::DescriptorMatcher> matcherTmp = cv::DescriptorMatcher::create(cv::DescriptorMatcher::FLANNBASED);
						std::vector< std::vector<cv::DMatch> > KNN_mTmp;
						std::vector<cv::DMatch> good_matchesTmp;
						matcherTmp->knnMatch(DataPointMiniMap, DataPointSomeMap, KNN_mTmp, 2);
						std::vector<double> lisx;
						std::vector<double> lisy;
						double sumx = 0;
						double sumy = 0;
						for (size_t i = 0; i < KNN_mTmp.size(); i++)
						{
							if (KNN_mTmp[i][0].distance < ratio_thresh * KNN_mTmp[i][1].distance)
							{
								good_matchesTmp.push_back(KNN_mTmp[i][0]);
								try 
								{
									// 这里有个bug回卡进来，进入副本或者切换放大招时偶尔触发
									lisx.push_back(((minMap.cols / 2 - KeyPointMiniMap[KNN_mTmp[i][0].queryIdx].pt.x)*mapScale + KeyPointSomeMap[KNN_mTmp[i][0].trainIdx].pt.x));
									lisy.push_back(((minMap.rows / 2 - KeyPointMiniMap[KNN_mTmp[i][0].queryIdx].pt.y)*mapScale + KeyPointSomeMap[KNN_mTmp[i][0].trainIdx].pt.y));

								}
								catch (...)
								{
									error_code = 7;//特征点数组访问越界，是个bug
									return false;
								}
								sumx += lisx.back();
								sumy += lisy.back();
							}
						}
#ifdef _DEBUG
						cv::Mat img_matches, imgmap, imgminmap;
						drawKeypoints(someMap, KeyPointSomeMap, imgmap, cv::Scalar::all(-1), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
						drawKeypoints(img_object, KeyPointMiniMap, imgminmap, cv::Scalar::all(-1), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
						drawMatches(img_object, KeyPointMiniMap, someMap, KeyPointSomeMap, good_matchesTmp, img_matches, cv::Scalar::all(-1), cv::Scalar::all(-1), std::vector<char>(), cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);
#endif
						if (lisx.size() <= 4 || lisy.size() <= 4)
						{
							isContinuity = false;
						}
						else
						{
							double meanx = sumx / lisx.size(); //均值
							double meany = sumy / lisy.size(); //均值
							cv::Point2f p = SPC(lisx, sumx, lisy, sumy);

							float x = (float)meanx;
							float y = (float)meany;

							x = p.x;
							y = p.y;

							pos = cv::Point2f(x + hisP[2].x - someSizeR, y + hisP[2].y - someSizeR);
						}
					}
				}
			}
			if (!isContinuity)
			{
				cv::Ptr<cv::xfeatures2d::SURF>& detectorAllMap = *(cv::Ptr<cv::xfeatures2d::SURF>*)_detectorAllMap;
				std::vector<cv::KeyPoint>& KeyPointAllMap = *(std::vector<cv::KeyPoint>*)_KeyPointAllMap;
				cv::Mat& DataPointAllMap = *(cv::Mat*)_DataPointAllMap;
				std::vector<cv::KeyPoint>& KeyPointMiniMap = *(std::vector<cv::KeyPoint>*)_KeyPointMiniMap;
				cv::Mat& DataPointMiniMap = *(cv::Mat*)_DataPointMiniMap;

				detectorAllMap->detectAndCompute(img_object, cv::noArray(), KeyPointMiniMap, DataPointMiniMap);
				cv::Ptr<cv::DescriptorMatcher> matcher = cv::DescriptorMatcher::create(cv::DescriptorMatcher::FLANNBASED);
				std::vector< std::vector<cv::DMatch> > KNN_m;
				//std::vector<DMatch> good_matches;
				matcher->knnMatch(DataPointMiniMap, DataPointAllMap, KNN_m, 2);

				std::vector<double> lisx;
				std::vector<double> lisy;
				double sumx = 0;
				double sumy = 0;
				for (size_t i = 0; i < KNN_m.size(); i++)
				{
					if (KNN_m[i][0].distance < ratio_thresh * KNN_m[i][1].distance)
					{
						//good_matches.push_back(KNN_m[i][0]);
						lisx.push_back(((img_object.cols / 2 - KeyPointMiniMap[KNN_m[i][0].queryIdx].pt.x)*mapScale + KeyPointAllMap[KNN_m[i][0].trainIdx].pt.x));
						lisy.push_back(((img_object.rows / 2 - KeyPointMiniMap[KNN_m[i][0].queryIdx].pt.y)*mapScale + KeyPointAllMap[KNN_m[i][0].trainIdx].pt.y));
						sumx += lisx.back();
						sumy += lisy.back();
					}
				}
				if (lisx.size() == 0 || lisy.size() == 0)
				{
					error_code = 4;//未能匹配到特征点
					return false;
				}
				else
				{
					pos = SPC(lisx, sumx, lisy, sumy);
				}
			}
			hisP[0] = hisP[1];
			hisP[1] = hisP[2];
			hisP[2] = pos;

			/******************************/

			x = (float)(pos.x);
			y = (float)(pos.y);

			error_code = 0;
			return true;
		}
		else
		{
			error_code = 3;//窗口画面为空
			return false;
		}
	}
	else
	{
		error_code = 2;//未能找到原神窗口句柄
		return false;
	}
}

bool AutoTrack::GetUID(int &uid)
{
	// 判断原神窗口不存在直接返回false，不对参数做任何修改
	if (getGengshinImpactWnd())
	{
		getGengshinImpactRect();
		getGengshinImpactScreen();

		if (!giFrame.empty())
		{
			getUIDRefMat();

			std::vector<cv::Mat> channels;

			split(giUIDRef, channels);
			giUIDRef = channels[3];

			int _uid = 0;
			int _NumBit[9] = { 0 };

			int bitCount = 1;
			cv::Mat matchTmp;
			cv::Mat Roi(giUIDRef);
			cv::Mat checkUID = giMatchResource.UID;
#ifdef _DEBUG
			//if (checkUID.rows > Roi.rows)
			//{
			//	cv::resize(checkUID, checkUID, cv::Size(), Roi.rows/ checkUID.rows);
			//}
#endif
			cv::matchTemplate(Roi, checkUID, matchTmp, cv::TM_CCOEFF_NORMED);

			double minVal, maxVal;
			cv::Point minLoc, maxLoc;
			//寻找最佳匹配位置
			cv::minMaxLoc(matchTmp, &minVal, &maxVal, &minLoc, &maxLoc);
			if (maxVal > 0.75)
			{
				int x = maxLoc.x + checkUID.cols + 7;
				int y = 0;
				double tmplis[10] = { 0 };
				int tmplisx[10] = { 0 };
				for (int p = 8; p >= 0; p--)
				{
					_NumBit[p] = 0;
					for (int i = 0; i < 10; i++)
					{
						cv::Rect r(x, y, giMatchResource.UIDnumber[i].cols + 2, giUIDRef.rows);//180-46/9->140/9->16->16*9=90+54=144
						if (x + r.width > giUIDRef.cols)
						{
							r = cv::Rect(giUIDRef.cols - giMatchResource.UIDnumber[i].cols - 2, y, giMatchResource.UIDnumber[i].cols + 2, giUIDRef.rows);
						}

						cv::Mat numCheckUID = giMatchResource.UIDnumber[i];
						Roi = giUIDRef(r);

						cv::matchTemplate(Roi, numCheckUID, matchTmp, cv::TM_CCOEFF_NORMED);

						double minVali, maxVali;
						cv::Point minLoci, maxLoci;
						//寻找最佳匹配位置
						cv::minMaxLoc(matchTmp, &minVali, &maxVali, &minLoci, &maxLoci);

						tmplis[i] = maxVali;
						tmplisx[i] = maxLoci.x + numCheckUID.cols - 1;
						if (maxVali > 0.91)
						{
							_NumBit[p] = i;
							x = x + maxLoci.x + numCheckUID.cols - 1;
							break;
						}
						if (i == 10 - 1)
						{
							_NumBit[p] = getMaxID(tmplis, 10);
							x = x + tmplisx[_NumBit[p]];
						}
					}
				}
			}
			_uid = 0;
			for (int i = 0; i < 9; i++)
			{
				_uid += _NumBit[i] * bitCount;
				bitCount = bitCount * 10;
			}

			if (_uid == 0)
			{
				error_code = 8;//未能在UID区域检测到有效UID
				return false;
			}

			uid = _uid;
			error_code = 0;
			return true;
		}
		else
		{
			error_code = 3;//窗口画面为空
			return false;
		}
	}
	else
	{
		error_code = 2;//未能找到原神窗口句柄
		return false;
	}
}

int AutoTrack::GetLastError()
{
	return error_code;
}

bool AutoTrack::getGengshinImpactWnd()
{
	giHandle = FindWindowA("UnityWndClass", "原神");/* 对原神窗口的操作 */
#ifdef _DEBUG
	std::cout << "GI Windows Handle Find is "<< giHandle << std::endl;
#endif
	return (giHandle != NULL ? true : false);
}

void AutoTrack::getGengshinImpactRect()
{
	GetWindowRect(giHandle, &giRect);
	GetClientRect(giHandle, &giClientRect);

	int x_offset = GetSystemMetrics(SM_CXDLGFRAME);
	int y_offset = GetSystemMetrics(SM_CYDLGFRAME) + GetSystemMetrics(SM_CYCAPTION);

	giClientSize.width = giClientRect.right - giClientRect.left;// -x_offset;
	giClientSize.height = giClientRect.bottom - giClientRect.top;// -y_offset;

#ifdef _DEBUG
	std::cout << "GI Windows Size: "<<giClientSize.width<<","<<giClientSize.height << std::endl;
#endif
}

void AutoTrack::getGengshinImpactScreen()
{
	static HBITMAP	hBmp;
	BITMAP bmp;

	DeleteObject(hBmp);

	if (giHandle == NULL)return;

	//获取目标句柄的窗口大小RECT
	GetWindowRect(giHandle, &giRect);/* 对原神窗口的操作 */

	//获取目标句柄的DC
	HDC hScreen = GetDC(giHandle);/* 对原神窗口的操作 */
	HDC hCompDC = CreateCompatibleDC(hScreen);

	//获取目标句柄的宽度和高度
	int	nWidth = giRect.right - giRect.left;
	int	nHeight = giRect.bottom - giRect.top;

	//创建Bitmap对象
	hBmp = CreateCompatibleBitmap(hScreen, nWidth, nHeight);//得到位图

	SelectObject(hCompDC, hBmp); //不写就全黑
	BitBlt(hCompDC, 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);

	//释放对象
	DeleteDC(hScreen);
	DeleteDC(hCompDC);

	//类型转换
	//这里获取位图的大小信息,事实上也是兼容DC绘图输出的范围
	GetObject(hBmp, sizeof(BITMAP), &bmp);

	int nChannels = bmp.bmBitsPixel == 1 ? 1 : bmp.bmBitsPixel / 8;
	int depth = bmp.bmBitsPixel == 1 ? IPL_DEPTH_1U : IPL_DEPTH_8U;

	//mat操作
	giFrame.create(cv::Size(bmp.bmWidth, bmp.bmHeight), CV_MAKETYPE(CV_8U, nChannels));

	GetBitmapBits(hBmp, bmp.bmHeight*bmp.bmWidth*nChannels, giFrame.data);

	giFrame = giFrame(cv::Rect(giClientRect.left, giClientRect.top, giClientSize.width, giClientSize.height));
}

void AutoTrack::getPaimonRefMat()
{
	int Paimon_Rect_x = cvCeil(giFrame.cols*0.0135);
	int Paimon_Rect_y = cvCeil(giFrame.cols*0.006075);
	int Paimon_Rect_w = cvCeil(giFrame.cols*0.035);
	int Paimon_Rect_h = cvCeil(giFrame.cols*0.0406);

	giPaimonRef = giFrame(cv::Rect(Paimon_Rect_x, Paimon_Rect_y, Paimon_Rect_w, Paimon_Rect_h));
#ifdef _DEBUG
	cv::imshow("Paimon", giPaimonRef);
	cv::waitKey(1);
	std::cout << "Show Paimon" << std::endl;
#endif
}

void AutoTrack::getMiniMapRefMat()
{
	int MiniMap_Rect_x = cvCeil(giFrame.cols*0.032);
	int MiniMap_Rect_y = cvCeil(giFrame.cols*0.01);
	int MiniMap_Rect_w = cvCeil(giFrame.cols*0.11);
	int MiniMap_Rect_h = cvCeil(giFrame.cols*0.11);

	giMiniMapRef = giFrame(cv::Rect(MiniMap_Rect_x, MiniMap_Rect_y, MiniMap_Rect_w, MiniMap_Rect_h));
#ifdef _DEBUG
	cv::imshow("MiniMap", giMiniMapRef);
	cv::waitKey(1);
	std::cout << "Show MiniMap" << std::endl;
#endif
}

void AutoTrack::getUIDRefMat()
{
	int UID_Rect_x = cvCeil(giFrame.cols*0.875);
	int UID_Rect_y = cvCeil(giFrame.rows*0.9755);
	int UID_Rect_w = cvCeil(giFrame.cols* 0.0938);
	int UID_Rect_h = cvCeil(UID_Rect_w*0.11);

	giUIDRef = giFrame(cv::Rect(UID_Rect_x, UID_Rect_y, UID_Rect_w, UID_Rect_h));
#ifdef _DEBUG
	cv::imshow("UID", giUIDRef);
	cv::waitKey(1);
	std::cout << "Show UID" << std::endl;
#endif
}
