#include <iostream>
using namespace std;
int main(){
    int M[2][3]={1,2,3,4,5}; 
    for(int i=0;i<2;i++){
        for(int j=0;j<3;j++){
            cout<<M[i][j]<<" ";
        }
        cout<<endl;
    }
}
