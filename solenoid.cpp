#include<cmath>
#include<cstdio>
#include<cassert>
#include<cstdlib>
#include<algorithm>
#include<iostream>
using namespace std;

template<typename ftype>
__device__ __forceinline__ void step(ftype u[3], ftype s[2], int n)
{
    const ftype c1 = 4.0, c2 = 2.0;
    for (int i = 0; i < n; ++i) {
        ftype x = u[0];
        ftype y = u[1];
        ftype z = u[2];
    
        ftype r2 = x * x + y * y;
        ftype r = sqrt(r2);
        ftype rtilde = s[0] + (r - s[0]) / c1 + x / c2 / r;
        ftype c2t = 2 * x * x / r2 - 1;
        ftype s2t = 2 * x * y / r2;
    
        ftype p = s[1]  * s2t * c2t / 2;
        ftype cp = cos(p);
        ftype sp = sin(p);
        ftype s2tp = s2t * cp + c2t * sp;
        ftype c2tp = c2t * cp - s2t * sp;
    
        u[0] = rtilde * c2tp;
        u[1] = rtilde * s2tp;
        u[2] = z / c1 + y / c2 / r;
    }
}


template<typename ftype>
__device__ __forceinline__ ftype objective(ftype u[3], ftype s[2])
{
	return u[0]*u[0] + u[1]*u[1];
}

template<typename ftype, typename objType>
__global__ void accumulate(ftype (*u)[3], ftype s[2], objType *obj, int steps)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    step(u[i], s, steps);
    ftype obji = objective(u[i], s);
    if (obji) atomicAdd(obj, obji);
}

typedef float ftype;
typedef float objType;

void init(ftype (**u)[3], ftype ** s, ftype s1, ftype s2, int nBlocks, int threadsPerBlock)
{
    const int nSamples = nBlocks * threadsPerBlock;
    ftype (*uCPU)[3] = new ftype[nSamples][3];
    for (int i = 0; i < nSamples; ++i) {
        uCPU[i][0] = 1.0; //rand() / (ftype)RAND_MAX;
        uCPU[i][1] = 1.0; //rand() / (ftype)RAND_MAX;
        uCPU[i][2] = 1.0; // rand() / (ftype)RAND_MAX;
    }
   
    cudaMalloc(u, sizeof(ftype) * nSamples * 3);
    cudaMemcpy(*u, uCPU, sizeof(ftype) * nSamples * 3, cudaMemcpyHostToDevice);
    delete[] uCPU;

    ftype sCPU[2] = {s1, s2};
    cudaMalloc(s, sizeof(ftype) * 2);
    cudaMemcpy(*s, sCPU, sizeof(ftype) * 2, cudaMemcpyHostToDevice);
   
    ftype (*uCPU2)[3] = new ftype[nSamples][3];
    cudaMemcpy(uCPU2, *u, sizeof(ftype) *nSamples * 3, cudaMemcpyDeviceToHost);
    cout << uCPU2[0][0] << " " << uCPU2[0][1] << " " << uCPU2[0][2] << '\n';

	ftype * objective;
    cudaMalloc(&objective, sizeof(ftype));
    cudaMemset(objective, 0.0, sizeof(ftype));

 
    ftype (*uCPU1)[3] = new ftype[nSamples][3];
    accumulate<<<nBlocks, threadsPerBlock>>>(*u, *s, objective, 1);
    cudaMemcpy(uCPU1, *u, sizeof(ftype) *nSamples * 3, cudaMemcpyDeviceToHost);
    cout << uCPU1[0][0] << " " << uCPU1[0][1] << " " << uCPU1[0][2] << '\n';
	ftype objCPU;
    cudaMemcpy(&objCPU, objective, sizeof(ftype), cudaMemcpyDeviceToHost);

    cout << objCPU << '\n';
    delete [] uCPU1;
    delete [] uCPU2;

}

int main(int argc, char * argv[])
{
    const int nBlocks = 32;
    const int threadsPerBlock = 256;
    assert (argc == 4);
    int iDevice = atoi(argv[1]);
    if (cudaSetDevice(iDevice)) {
        fprintf(stderr, "error selecting %d\n", iDevice);
        exit(-1);
    }
	ftype s1 = atof(argv[2]);
	ftype s2 = atof(argv[3]); 
	

    ftype (*u)[3], *s;
    init(&u, &s, s1, s2, nBlocks, threadsPerBlock);
    ftype *objCPU;
    double *objFinal = new double;
    memset(objFinal, 0, sizeof(double));

    ftype * objective;
    cudaMalloc(&objective, sizeof(ftype));
    /*
    const int nRepeat = 1;
    for (int iRepeat = 0; iRepeat < nRepeat; ++iRepeat) {
        cudaMemset(objective, 0.0, sizeof(ftype));

      cout << "hello" << '\n';
 
       cudaMemcpy(objCPU, objective, sizeof(ftype), cudaMemcpyDeviceToHost);
    
        
            *objFinal += *objCPU / nRepeat / nBlocks / threadsPerBlock;
        
     printf("%40.30f\n", *objFinal);
        
	} */
}
