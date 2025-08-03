#ifndef __UTILS_H__
#define __UTILS_H__

#ifndef DEBUG
#define DEBUG (4*1+1) // 4: error, 3: info, 2: debug, 1: verbose
#endif // DEBUG
#if DEBUG==7
    #define vprint(...) printf("[VERB]  : %15s:%-4d : ",__FILE__, __LINE__); printf(__VA_ARGS__)
    #define dprint(...) printf("[DEBUG] : %15s:%-4d : ",__FILE__, __LINE__); printf(__VA_ARGS__)
    #define iprint(...) printf("[INFO]  : %15s:%-4d : ",__FILE__, __LINE__); printf(__VA_ARGS__)
    #define eprint(...) printf("[ERROR] : %15s:%-4d : ",__FILE__, __LINE__); printf(__VA_ARGS__)
#elif DEBUG==6
    #define vprint(...) 
    #define dprint(...) printf("[DEBUG] : %15s:%-4d : ",__FILE__, __LINE__); printf(__VA_ARGS__)
    #define iprint(...) printf("[INFO]  : %15s:%-4d : ",__FILE__, __LINE__); printf(__VA_ARGS__)
    #define eprint(...) printf("[ERROR] : %15s:%-4d : ",__FILE__, __LINE__); printf(__VA_ARGS__)
#elif DEBUG==5
    #define vprint(...)
    #define dprint(...) 
    #define iprint(...) printf("[INFO]  : %15s:%-4d : ",__FILE__, __LINE__); printf(__VA_ARGS__)
    #define eprint(...) printf("[ERROR] : %15s:%-4d : ",__FILE__, __LINE__); printf(__VA_ARGS__)
#elif DEBUG==4
    #define vprint(...)
    #define dprint(...)
    #define iprint(...)
    #define eprint(...) printf("[ERROR] : %15s:%-4d : ",__FILE__, __LINE__); printf(__VA_ARGS__)
#elif DEBUG==3
    #define vprint(...) printf("[VERB]  : %20s : ",__func__); printf(__VA_ARGS__)
    #define dprint(...) printf("[DEBUG] : %20s : ",__func__); printf(__VA_ARGS__)
    #define iprint(...) printf("[INFO]  : %20s : ",__func__); printf(__VA_ARGS__)
    #define eprint(...) printf("[ERROR] : %20s : ",__func__); printf(__VA_ARGS__)
#elif DEBUG==2
    #define vprint(...) 
    #define dprint(...) printf("[DEBUG] : %20s : ",__func__); printf(__VA_ARGS__)
    #define iprint(...) printf("[INFO]  : %20s : ",__func__); printf(__VA_ARGS__)
    #define eprint(...) printf("[ERROR] : %20s : ",__func__); printf(__VA_ARGS__)
#elif DEBUG==1
    #define vprint(...)
    #define dprint(...) 
    #define iprint(...) printf("[INFO]  : %20s : ",__func__); printf(__VA_ARGS__)
    #define eprint(...) printf("[ERROR] : %20s : ",__func__); printf(__VA_ARGS__)
#elif DEBUG==0
    #define vprint(...)
    #define dprint(...)
    #define iprint(...)
    #define eprint(...) printf("[ERROR] : %20s : ",__func__); printf(__VA_ARGS__)
#endif

#endif //__UTILS_H__