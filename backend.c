#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "backend.h"

// ===================== INITIAL FOOD LIST =====================
Food foods[50] = {
    {"F001", "Biryani", 300.0},
    {"F002", "Pizza", 250.0},
    {"F003", "Butter Chicken", 280.0},
    {"F004", "Dosa", 80.0},
    {"F005", "Naan", 50.0},
    {"F006", "Samosa", 20.0},
    {"F007", "Chole Bhature", 120.0},
    {"F008", "Paneer Tikka", 200.0},
    {"F009", "Fried Rice", 150.0},
    {"F010", "Tandoori Chicken", 350.0}
};

int food_count = 10;

Queue order_queue;
Stack cancel_stack;
PriorityQueue vip_queue;
int global_order_id = 1000;

// ===================== STACK FUNCTIONS =====================

void stack_init(Stack *s) {
    s->top = NULL;
    s->size = 0;
}

void stack_push(Stack* s, OrderItem item) {
    StackNode* n = (StackNode*)malloc(sizeof(StackNode));
    n->item = item;
    n->next = s->top;
    s->top = n;
    s->size++;
}

OrderItem* stack_pop(Stack* s) {
    if (s->size == 0) return NULL;

    StackNode* temp = s->top;
    OrderItem* it = malloc(sizeof(OrderItem));
    *it = temp->item;

    s->top = temp->next;
    free(temp);
    s->size--;

    return it;
}

// ===================== QUEUE FUNCTIONS =====================

void queue_init(Queue *q) {
    q->front = q->rear = NULL;
    q->size = 0;
}

void queue_enqueue(Queue* q, QueueNode* node) {
    node->next = NULL;
    if (!q->front) {
        q->front = q->rear = node;
    } else {
        q->rear->next = node;
        q->rear = node;
    }
    q->size++;
}

QueueNode* queue_dequeue(Queue* q) {
    if (!q->front) return NULL;

    QueueNode* temp = q->front;
    q->front = temp->next;

    if (!q->front) q->rear = NULL;

    q->size--;
    return temp;
}

// ===================== PRIORITY QUEUE FUNCTIONS =====================

void pqueue_init(PriorityQueue *pq) {
    pq->head = NULL;
    pq->size = 0;
}

void pqueue_insert(PriorityQueue *pq, QueueNode order, int priority) {
    PriorityQueueNode* n = malloc(sizeof(PriorityQueueNode));
    n->order = order;
    n->priority = priority;
    n->next = NULL;

    if (!pq->head || pq->head->priority < priority) {
        n->next = pq->head;
        pq->head = n;
        return;
    }

    PriorityQueueNode* cur = pq->head;
    while (cur->next && cur->next->priority >= priority)
        cur = cur->next;

    n->next = cur->next;
    cur->next = n;
}

PriorityQueueNode* pqueue_pop(PriorityQueue *pq) {
    if (!pq->head) return NULL;

    PriorityQueueNode* temp = pq->head;
    pq->head = pq->head->next;

    return temp;
}

// ===================== VIP CHECK =====================

int is_vip_customer(const char *phone) {
    FILE *fp = fopen("vip.txt", "r");
    if (!fp) return 0;

    char line[50];
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;
        if (strcmp(line, phone) == 0) {
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    return 0;
}

// ===================== WRITE ORDER TO FILE =====================

void save_order_to_file(QueueNode *o) {
    FILE *fp = fopen("data.txt", "a");
    if (!fp) return;

    fprintf(fp, "\n===== ORDER #%d =====\n", o->order_id);
    fprintf(fp, "Customer: %s\n", o->customer_name);
    fprintf(fp, "Phone: %s\n", o->phone);
    fprintf(fp, "VIP: %s\n", o->is_vip ? "YES" : "NO");

    for (int i = 0; i < o->item_count; i++) {
        fprintf(fp, "%s x %d = %.2f\n",
            o->items[i].food_name,
            o->items[i].quantity,
            o->items[i].subtotal
        );
    }

    fprintf(fp, "Total: %.2f\n", o->total);
    fclose(fp);
}

// ===================== ADD ORDER (GUI Version) =====================

OrderResult add_order_gui(const char *customer, const char *phone,
                          const char *food_id, int quantity) {

    OrderResult r = {0, 0, 0.0};

    // Find food
    Food *f = NULL;
    for (int i = 0; i < food_count; i++) {
        if (strcmp(foods[i].id, food_id) == 0) {
            f = &foods[i];
            break;
        }
    }

    if (!f) return r;  // not found

    QueueNode *node = malloc(sizeof(QueueNode));

    strcpy(node->customer_name, customer);
    strcpy(node->phone, phone);
    node->item_count = 1;
    node->timestamp = time(NULL);
    node->order_id = global_order_id++;

    strcpy(node->items[0].food_id, food_id);
    strcpy(node->items[0].food_name, f->name);
    node->items[0].quantity = quantity;
    node->items[0].price = f->price;
    node->items[0].subtotal = f->price * quantity;

    node->total = node->items[0].subtotal;

    node->is_vip = is_vip_customer(phone);
    node->priority = node->is_vip ? 10 : 1;

    // insert into correct queue
    if (node->is_vip)
        pqueue_insert(&vip_queue, *node, node->priority);
    else
        queue_enqueue(&order_queue, node);

    // push for undo
    stack_push(&cancel_stack, node->items[0]);

    // save to file
    save_order_to_file(node);

    r.success = 1;
    r.order_id = node->order_id;
    r.total = node->total;

    return r;
}

// ===================== CANCEL RECENT ITEM =====================

CancelResult cancel_recent_gui() {
    CancelResult r = {0};

    OrderItem *it = stack_pop(&cancel_stack);
    if (!it) return r;

    r.success = 1;
    strcpy(r.food_name, it->food_name);
    r.quantity = it->quantity;
    r.subtotal = it->subtotal;

    free(it);
    return r;
}

// ===================== PROCESS NEXT ORDER =====================

ProcessResult process_next_order_gui() {
    ProcessResult r = {0};

    QueueNode *order = NULL;

    if (vip_queue.head) {
        PriorityQueueNode *pn = pqueue_pop(&vip_queue);
        order = malloc(sizeof(QueueNode));
        *order = pn->order;
        free(pn);
    } else if (order_queue.front) {
        order = queue_dequeue(&order_queue);
    }

    if (!order)
        return r;

    r.success = 1;
    r.order_id = order->order_id;
    strcpy(r.customer, order->customer_name);
    r.total = order->total;
    r.is_vip = order->is_vip;

    free(order);
    return r;
}

// ===================== SORTING FUNCTIONS (unchanged) =====================

void sort_foods_by_price_asc(Food* arr, int n) {
    for (int i = 0; i < n - 1; i++)
        for (int j = 0; j < n - i - 1; j++)
            if (arr[j].price > arr[j + 1].price) {
                Food tmp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = tmp;
            }
}

void sort_foods_by_price_desc(Food* arr, int n) {
    for (int i = 0; i < n - 1; i++)
        for (int j = 0; j < n - i - 1; j++)
            if (arr[j].price < arr[j + 1].price) {
                Food tmp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = tmp;
            }
}

// ===================== INITIALIZE ALL STRUCTURES =====================

void initialize_backend() {
    queue_init(&order_queue);
    stack_init(&cancel_stack);
    pqueue_init(&vip_queue);
}
