#include <gtk/gtk.h>
#include "backend.h"   // contains your DSA code

// ==================== GLOBAL WIDGETS ==========================
GtkWidget *food_list_view;
GtkWidget *order_list_view;
GtkWidget *entry_name;
GtkWidget *entry_phone;
GtkWidget *entry_food_id;
GtkWidget *entry_quantity;
GtkWidget *status_label;

// ===============================================================
//      UTILITY: Show message in a pop-up dialog
// ===============================================================
void show_message(GtkWindow *parent, const char *message) {
    GtkWidget *dialog = gtk_message_dialog_new(
        parent,
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "%s",
        message
    );
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// ===============================================================
//                     DISPLAY FOOD LIST
// ===============================================================
void update_food_list() {
    GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(food_list_view)));
    gtk_list_store_clear(store);

    for (int i = 0; i < food_count; i++) {
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
            0, foods[i].id,
            1, foods[i].name,
            2, foods[i].price,
            -1
        );
    }
}

// ===============================================================
//              DISPLAY PENDING ORDERS (REG + VIP)
// ===============================================================
void update_order_list() {
    GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(order_list_view)));
    gtk_list_store_clear(store);

    // ---------- Regular Queue ----------
    QueueNode *current = order_queue.front;
    while (current) {
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
            0, current->order_id,
            1, current->customer_name,
            2, current->phone,
            3, current->total,
            4, current->is_vip ? "YES" : "NO",
            -1
        );
        current = current->next;
    }

    // ---------- VIP Priority Queue ----------
    PriorityQueueNode *vip = vip_queue.head;
    while (vip) {
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
            0, vip->order.order_id,
            1, vip->order.customer_name,
            2, vip->order.phone,
            3, vip->order.total,
            4, "VIP",
            -1
        );
        vip = vip->next;
    }
}

// ===============================================================
//                    ADD ORDER BUTTON HANDLER
// ===============================================================
void on_add_order(GtkButton *btn, gpointer user_data) {

    const char *name = gtk_entry_get_text(GTK_ENTRY(entry_name));
    const char *phone = gtk_entry_get_text(GTK_ENTRY(entry_phone));
    const char *fid = gtk_entry_get_text(GTK_ENTRY(entry_food_id));
    int qty = atoi(gtk_entry_get_text(GTK_ENTRY(entry_quantity)));

    if (strlen(name) == 0 || strlen(phone) == 0 || strlen(fid) == 0 || qty <= 0) {
        show_message(GTK_WINDOW(user_data), "Please enter valid details.");
        return;
    }

    // Call your existing backend logic:
    OrderResult result = add_order_gui(name, phone, fid, qty);

    if (result.success) {
        char msg[256];
        sprintf(msg, "Order Added\nOrder ID: %d\nTotal: %.2f", result.order_id, result.total);
        show_message(GTK_WINDOW(user_data), msg);

        update_order_list();
    } else {
        show_message(GTK_WINDOW(user_data), "Error: Food not found.");
    }
}

// ===============================================================
//                    CANCEL RECENT ITEM
// ===============================================================
void on_cancel_recent(GtkButton *btn, gpointer window) {
    CancelResult result = cancel_recent_gui();

    if (!result.success) {
        show_message(GTK_WINDOW(window), "No items to cancel.");
        return;
    }

    char msg[256];
    sprintf(msg, "Cancelled: %s (Qty: %d)", result.food_name, result.quantity);
    show_message(GTK_WINDOW(window), msg);
}

// ===============================================================
//                    PROCESS NEXT ORDER
// ===============================================================
void on_process_order(GtkButton *btn, gpointer window) {
    ProcessResult r = process_next_order_gui();

    if (!r.success) {
        show_message(GTK_WINDOW(window), "No orders to process.");
        return;
    }

    char msg[256];
    sprintf(msg, "Processed Order #%d\nCustomer: %s", r.order_id, r.customer);
    show_message(GTK_WINDOW(window), msg);

    update_order_list();
}

// ===============================================================
//                          BUILD UI
// ===============================================================
GtkWidget* build_ui(GtkApplication *app) {
    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *button_add, *button_cancel, *button_process;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Food Management System");
    gtk_window_set_default_size(GTK_WINDOW(window), 900, 600);

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_container_add(GTK_CONTAINER(window), grid);

    // ----------- Input Fields ---------
    entry_name = gtk_entry_new();
    entry_phone = gtk_entry_new();
    entry_food_id = gtk_entry_new();
    entry_quantity = gtk_entry_new();

    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_name), "Customer Name");
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_phone), "Phone");
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_food_id), "Food ID (F001)");
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_quantity), "Quantity");

    gtk_grid_attach(GTK_GRID(grid), entry_name, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_phone, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_food_id, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_quantity, 0, 3, 1, 1);

    // ------------ Buttons ---------------
    button_add = gtk_button_new_with_label("Add Order");
    button_cancel = gtk_button_new_with_label("Cancel Recent Item");
    button_process = gtk_button_new_with_label("Process Next Order");

    gtk_grid_attach(GTK_GRID(grid), button_add, 0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), button_cancel, 0, 5, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), button_process, 0, 6, 1, 1);

    // Connect signals
    g_signal_connect(button_add, "clicked", G_CALLBACK(on_add_order), window);
    g_signal_connect(button_cancel, "clicked", G_CALLBACK(on_cancel_recent), window);
    g_signal_connect(button_process, "clicked", G_CALLBACK(on_process_order), window);

    // =================== FOOD LIST TABLE =====================
    GtkListStore *food_store =
        gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_FLOAT);

    food_list_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(food_store));

    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(food_list_view), 0, "ID",
                                                gtk_cell_renderer_text_new(), "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(food_list_view), 1, "Name",
                                                gtk_cell_renderer_text_new(), "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(food_list_view), 2, "Price",
                                                gtk_cell_renderer_text_new(), "text", 2, NULL);

    gtk_grid_attach(GTK_GRID(grid), food_list_view, 1, 0, 1, 5);

    // =================== ORDERS TABLE =====================
    GtkListStore *order_store =
        gtk_list_store_new(5, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_FLOAT, G_TYPE_STRING);

    order_list_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(order_store));

    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(order_list_view), 0, "Order ID",
                                                gtk_cell_renderer_text_new(), "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(order_list_view), 1, "Customer",
                                                gtk_cell_renderer_text_new(), "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(order_list_view), 2, "Phone",
                                                gtk_cell_renderer_text_new(), "text", 2, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(order_list_view), 3, "Total",
                                                gtk_cell_renderer_text_new(), "text", 3, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(order_list_view), 4, "VIP",
                                                gtk_cell_renderer_text_new(), "text", 4, NULL);

    gtk_grid_attach(GTK_GRID(grid), order_list_view, 1, 5, 1, 5);

    // Load initial data
    update_food_list();
    update_order_list();

    return window;
}

// ===============================================================
//                     APPLICATION ENTRY
// ===============================================================
static void activate(GtkApplication *app, gpointer data) {
    GtkWidget *window = build_ui(app);
    gtk_widget_show_all(window);
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    // initialize backend
    initialize_backend();

    app = gtk_application_new("com.food.manager", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
