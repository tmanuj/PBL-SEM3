#ifndef BACKEND_H
#define BACKEND_H

#include <time.h>

// ===================== STRUCTS FROM ORIGINAL CODE =====================

typedef struct {
    char id[10];
    char name[50];
    float price;
} Food;

typedef struct {
    char food_id[10];
    char food_name[50];
    int quantity;
    float price;
    float subtotal;
} OrderItem;

typedef struct StackNode {
    OrderItem item;
    struct StackNode* next;
} StackNode;

typedef struct {
    StackNode* top;
    int size;
} Stack;

typedef struct QueueNode {
    char customer_name[50];
    char phone[15];
    OrderItem items[20];
    int item_count;
    float total;
    int is_vip;
    int priority;
    time_t timestamp;
    int order_id;
    struct QueueNode* next;
} QueueNode;

typedef struct {
    QueueNode* front;
    QueueNode* rear;
    int size;
} Queue;

typedef struct PriorityQueueNode {
    QueueNode order;
    int priority;
    struct PriorityQueueNode* next;
} PriorityQueueNode;

typedef struct {
    PriorityQueueNode* head;
    int size;
} PriorityQueue;

// ===================== RESULT STRUCTS FOR GTK =====================

typedef struct {
    int success;
    int order_id;
    float total;
} OrderResult;

typedef struct {
    int success;
    char food_name[50];
    int quantity;
    float subtotal;
} CancelResult;

typedef struct {
    int success;
    int order_id;
    char customer[50];
    float total;
    int is_vip;
} ProcessResult;

// ===================== GLOBAL BACKEND VARIABLES =====================
extern Food foods[50];
extern int food_count;

extern Queue order_queue;
extern Stack cancel_stack;
extern PriorityQueue vip_queue;
extern int global_order_id;

// ===================== BACKEND FUNCTIONS =====================

// Must be called once before GTK app starts
void initialize_backend();

OrderResult add_order_gui(const char *customer, const char *phone,
                          const char *food_id, int quantity);

CancelResult cancel_recent_gui();

ProcessResult process_next_order_gui();

// Sorting functions
void sort_foods_by_price_asc(Food*, int);
void sort_foods_by_price_desc(Food*, int);

// Helper
int is_vip_customer(const char *phone);

#endif
