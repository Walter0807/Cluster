// Clustering.cpp : Defines the entry point for the console application.
//
#include <cstdio>
#include <cstdlib> 
#include <cstring>
#include <cmath>
#include <ctime>
#define For(i,a,b) for(int i=a;i<=b;i++)

# define MAXNUMFEATURE 20
# define MAXNUMTEXT 200
# define MAXNUMCATEGORY 20
# define MAXNUMCLUSTER 200
int INT_MAX = 30000000;

typedef struct
{
	double  feature[MAXNUMFEATURE];  //属性数组
	int  categories;	             //文本的预定义类，用作聚类的质量评测
	int	 cluster;		             //文本所属的聚类号
}Text;

typedef struct
{
	double feature[MAXNUMFEATURE];	//质心向量
	int elementsCount;			    //类中文档数目
	int elements[MAXNUMTEXT];	    //类中文档ID号
}Cluster;

void K_Means_Clustering();
void DisplayClusters();

int indexOf(char *);  //输入类名称，输出类标号

#define K 7                         // 设定为7个类进行K-means聚类
#define RSS_THRESHOLD 1e-2			// RSS停止阈值
#define MAX_ITERATION_NUM 50		// 最大迭代次数

int numFeatures = 0;                   
int numClusters = 0;                //初始值为0

Cluster clusters[MAXNUMTEXT];       //cluster: {质心向量；类中文档数目；类中文档ID号}

char categories[MAXNUMCATEGORY][100];  //字符串数组，存放类型的名称
int numcate[20],Pcount[20],numClu[20];
double Pur[20];
int numCategories = 0;                 //实际分类的数目

Text texts[MAXNUMTEXT];               //Text：{属性数组；文本的预定义类，用作聚类的质量评测；文本所属的聚类号}
int numTexts = 0;

double prev_RSS = 0;                  //前一次的迭代的平方距离残量，初始为0
int iteration = 1;
int N[12][12];
double dis[200][200];
double tot[200];


//从数据文件中读入文档特征向量数据
bool input()
{
	FILE *file = fopen("data.txt","r"); //打开数据文件
	if(file == NULL)
	{
		fprintf(stderr,"[ERROR]: unable to open the data file\n");
		return false;
	}

	fscanf(file,"%d %d %d",&numTexts,&numFeatures,&numCategories); // 数据文件第一行：文档数，特征数，分类数

	for(int i = 0 ; i < numCategories; i ++) // 读入类型名称: games IT sports oversea entertainment military  political
		fscanf(file,"%s",categories[i]);   

	char tmp[100];                           // 特征向量的分类标记（用于评测聚类结果）                
	for(int i = 0 ; i < numTexts; i ++)      
	{
		for(int j = 0 ; j < numFeatures - 1; j ++)  // 读入一条特征向量
			fscanf(file,"%lf",&texts[i].feature[j]);           

		fscanf(file,"%s",tmp);                      //读入该向量的分类标记名称

		int index = indexOf(tmp);                  // 取得该标记对应的分类号
		if(-1 == index)
			return false;
		
		texts[i].categories = index;              //设置分类属性
	}
	return true;
}

//计算两个特征向量（两篇文档或者一篇文档和一个类的质心）之间的平方欧氏距离
double getSquaredEuclideanDist(double features1[], double features2[])
{
	double dist = 0;

	for (int i=0; i<numFeatures-1; i++) 
	{
		dist += ((features1[i] - features2[i])*(features1[i] - features2[i])); 
	}
	return dist;
}

//计算两个文档（或者一篇文档和一个类的质心）之间的欧氏距离
double getEuclideanDist(double features1[], double features2[]) 
{
	return sqrt(getSquaredEuclideanDist(features1, features2));
}

void Calcdis()
{
	For(i,0,numTexts-1)
	For(j,0,numTexts-1)
	dis[i][j] = getSquaredEuclideanDist(texts[i].feature, texts[j].feature);
//	For(i,0,numTexts-1) For(j,0,numTexts-1) printf("%lf\n", dis[i][j]);
}

//从n篇文档(文档号：0~n-1)中随机选择k个(不重复的数)作为初始聚类种子
void selectRandomSeeds(int n, int k, int seeds[])
{
	Calcdis();
	srand(time(NULL));   //生成伪随机数种子 
	int r = rand() % n;   //生成 0到index-1之间的一个随机数
	int max2;
	seeds[0] = r;    
	For(i,0,numTexts-1) tot[i] = dis[r][i];
	tot[r] = -1;
//	printf("Random Seeds: ");
	for (int i=1; i<k; i++) 
	{
		max2 = 0;
		For(j,0,numTexts-1) 
		if(tot[j]>max2)
		{
			max2 = tot[j];
			r = j;
		} 
		seeds[i] = r;    
		For(j,0,numTexts-1) tot[j] = (tot[j]*dis[r][j])/(tot[j]+dis[r][j]);
		tot[r] = -1;
	}
//	For(i,0,k-1) printf("Seed%d:%d\n", i, seeds[i]);

}

//计算Residual Sum of Square: the squared distance of each vector
//from its centroid summed over all vectors.
double calculateRSS()
{
	double *RSS = new double[numClusters];        // 每个类计算一个RSS值
	double totalRSS = 0.0;            

	memset(RSS, 0, sizeof(double) * numClusters);//初始化RSS值数组

	for (int i=0; i<numTexts; i++)  //每个样本算一下跟自己归的类的质心之间的平方距离
	{
		int clusterID = texts[i].cluster;
		RSS[clusterID] += getSquaredEuclideanDist(texts[i].feature, clusters[clusterID].feature);
	}

	for (int i=0; i<numClusters; i++)  //平方距离总和
		totalRSS += RSS[i];

	delete []RSS;
	return totalRSS;
}

//判断迭代中止条件：RSS值变化幅度小于阈值RSS_THRESHOLD，或者迭代次数大于最大允许次数
bool converge()
{
	bool flag = false;
	double cur_RSS = calculateRSS();
//	printf ("RSS: %lf\n", cur_RSS);

	if (abs(cur_RSS - prev_RSS) < RSS_THRESHOLD || iteration > MAX_ITERATION_NUM)
		flag = true;

	prev_RSS = cur_RSS;
	return flag;
}

//对每个文本进行类的赋值，将它分到质心距离它最近的类中
void reassignment()
{
	int i, j;

	for (j=0; j<numClusters; j++)
		clusters[j].elementsCount = 0;

	for (i=0; i<numTexts; i++)    //为每个样本找归属类
	{
		double minDist = double(INT_MAX); //初始设定距离为INT_MAX
		texts[i].cluster = -1;

		for (j=0; j<numClusters; j++)   //计算跟所有的类之间的距离，选定距离最小的那个类
		{
			double dist = getEuclideanDist(texts[i].feature, clusters[j].feature); // 计算欧氏距离
			if (dist < minDist)       //距离较小？
			{
				minDist = dist;       //记录距离
				texts[i].cluster = j; //归入该类
			}
		}
		//在分到的类中添加新加入的文本ID号
		int count = clusters[ texts[i].cluster ].elementsCount; //取得该类目前的文档数
		clusters[ texts[i].cluster ].elements[count] = i;       //在对应位置记录新加入的文档ID
		clusters[ texts[i].cluster ].elementsCount ++;          //文档数++
	}
}

//更新各个类的质心
void updateClusterCentroid()
{
	int i, j, k;
	for (i=0; i<numClusters; i++)  //每个类跟新
	{
		if (clusters[i].elementsCount == 0)   //没有元素就pass
			continue;

		memset(clusters[i].feature, 0, sizeof(double) * (numFeatures-1));//清空原质心向量

		for (j=0; j<clusters[i].elementsCount; j++)  //对类中的所有样本的向量加总 平均
			for (k=0; k<numFeatures-1; k++)          //按每个特征依次进行加总
				clusters[i].feature[k] += texts[ clusters[i].elements[j] ].feature[k];

		for (k=0; k<numFeatures-1; k++)
			clusters[i].feature[k] /= clusters[i].elementsCount;  //平均
	}
}

void K_Means_Clustering()
{
	numClusters = K;  //不同的K值效果会不同，可以多次尝试最后用一个最佳的结果
	
	int i, seeds[MAXNUMCLUSTER];
	selectRandomSeeds(numTexts, numClusters, seeds);//随机选K篇文档作为类的质心
	for (i=0; i<numClusters; i++)
		memcpy(clusters[i].feature, texts[ seeds[i] ].feature,
		                                    sizeof(double) * (numFeatures-1));

	while (!converge())
	{
	//	printf ("Iteration: %d\n", iteration);

		reassignment();  //对每个文本进行类的赋值
		
		updateClusterCentroid();//更新各个类的质心
		
		iteration ++;
	}
}

void CountIndex()
{
	for(int i = 0 ; i < numTexts; i ++)   
	{
		numcate[texts[i].categories]++;
		numClu[texts[i].cluster]++;
	}
	//For(i,0,numCategories-1) printf("%d\n", numcate[i]);
}


void CalcF()
{
	CountIndex();
	int tmp,ori,cur;
	double rec, prec, max1, F, tmpp, ans;;
	int ni,nr;
	F = 0;
	ans = 0;
	for (int i=0; i<numClusters; i++)  //按每个类
	{
		tmp = 0;
		memset(Pcount,0,sizeof(Pcount));
		for (int k=0; k<clusters[i].elementsCount; k++) //所包含的文档编号
		{
			cur = clusters[i].elements[k];
			Pcount[texts[cur].categories]++;
			N[texts[cur].categories][i]++;

		}
			for (int j=0; j<numCategories; j++)	
			if(Pcount[j]>tmp) 
			{
				tmp = Pcount[j];
				ori = j;
			}

		Pur[i] = tmp/(double)numcate[ori];
		ans+=Pur[i];
		//printf("ID=%d,original=%d,times=%d,Purity =%lf\n", i, ori, tmp, Pur[i]);
	}
	tmpp = 0;
	For(i,0,numCategories-1)
	{
		max1 = -1;
		ni = numcate[i];
		For(r,0,numClusters-1)
		{
			if(N[i][r]==0) continue;
			nr = numClu[r];
			prec = N[i][r]/(double)nr;
			rec = N[i][r]/(double)ni;
			tmpp = ((2*rec*prec)/(prec+rec));
			if(tmpp>max1) max1 = tmpp;
		}
		if(max1!=-1) F += ni*max1;

	}
	//printf("F = %lf\n", F);
	F = F/(double)numTexts;
	ans = ans/(double)numClusters;
	printf("F = %lf, Average Purity = %lf\n", F, ans);

}





int main()
{
	freopen("1.txt","a",stdout);
	if(!input())
		return -1;

	K_Means_Clustering(); //按最小平均距离K聚类
//	DisplayClusters();    //显示聚类结果
	CalcF();
	return 0;
}


int indexOf(char * t)    //输入类名称，返回类标号
{
	if(t == NULL)
	{
		fprintf(stderr,"[WARING]: attempt to find index of NULL in category array\n");
		return -1;
	}

	for(int i = 0; i < numCategories; i ++)
	{
		if(0 == strcmp(categories[i],t))
			return i;
	}
	
	fprintf(stderr,"[WARING]: unable to find index of %s\n",t);
	return -1;
}
//显示所有的类的质心以及类的文档号
void DisplayClusters()
{
	int i, j, k;
	for (i=0; i<numClusters; i++)  //按每个类
	{
		printf("Cluster ID %d:\n", i);
		printf("Centroid: ");
		for (j=0; j<numFeatures-1; j++)   //重心向量
			printf("%lf ", clusters[i].feature[j]);
			printf("\n");printf("Elements ");
			for (k=0; k<clusters[i].elementsCount; k++) //所包含的文档编号
			printf("%d ", clusters[i].elements[k]);
		  printf("\n");
	}
}







