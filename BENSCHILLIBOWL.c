/*
 * Project #2 - Part 2: Ben's Chili Bowl (Thread Synchronization)
 * Authors: Danny Toledo-Navarro, [Partner name, if any]
 * Outside assistance: Claude (Anthropic AI assistant) helped draft this code.
 */
#include "BENSCHILLIBOWL.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

bool IsEmpty(BENSCHILLIBOWL* bcb);
bool IsFull(BENSCHILLIBOWL* bcb);
void AddOrderToBack(Order **orders, Order *order);

MenuItem BENSCHILLIBOWLMenu[] = {
    "BensChilli",
    "BensHalfSmoke",
    "BensHotDog",
    "BensChilliCheeseFries",
    "BensShake",
    "BensHotCakes",
    "BensCake",
    "BensHamburger",
    "BensVeggieBurger",
    "BensOnionRings",
};
int BENSCHILLIBOWLMenuLength = 10;

/* Select a random item from the Menu and return it */
MenuItem PickRandomMenuItem() {
    return BENSCHILLIBOWLMenu[rand() % BENSCHILLIBOWLMenuLength];
}

/* Allocate memory for the Restaurant, then create the mutex and condition variables needed to instantiate the Restaurant */

BENSCHILLIBOWL* OpenRestaurant(int max_size, int expected_num_orders) {
    BENSCHILLIBOWL *bcb = (BENSCHILLIBOWL*) malloc(sizeof(BENSCHILLIBOWL));

    bcb->orders = NULL;
    bcb->current_size = 0;
    bcb->max_size = max_size;
    bcb->next_order_number = 1;
    bcb->orders_handled = 0;
    bcb->expected_num_orders = expected_num_orders;

    pthread_mutex_init(&(bcb->mutex), NULL);
    pthread_cond_init(&(bcb->can_add_orders), NULL);
    pthread_cond_init(&(bcb->can_get_orders), NULL);

    srand(time(NULL));
    printf("Restaurant is open!\n");
    return bcb;
}


/* check that the number of orders received is equal to the number handled (ie.fullfilled). Remember to deallocate your resources */

void CloseRestaurant(BENSCHILLIBOWL* bcb) {
    assert(bcb->orders_handled == bcb->expected_num_orders);

    pthread_mutex_destroy(&(bcb->mutex));
    pthread_cond_destroy(&(bcb->can_add_orders));
    pthread_cond_destroy(&(bcb->can_get_orders));
    free(bcb);

    printf("Restaurant is closed!\n");
}

/* add an order to the back of queue */
int AddOrder(BENSCHILLIBOWL* bcb, Order* order) {
    int order_number;

    pthread_mutex_lock(&(bcb->mutex));

    // wait until there is room in the restaurant
    while (IsFull(bcb)) {
        pthread_cond_wait(&(bcb->can_add_orders), &(bcb->mutex));
    }

    order->order_number = bcb->next_order_number;
    order_number = order->order_number;
    bcb->next_order_number++;
    AddOrderToBack(&(bcb->orders), order);
    bcb->current_size++;

    // let a cook know there is an order
    pthread_cond_signal(&(bcb->can_get_orders));
    pthread_mutex_unlock(&(bcb->mutex));
    return order_number;
}

/* remove an order from the queue */
Order *GetOrder(BENSCHILLIBOWL* bcb) {
    Order *order;

    pthread_mutex_lock(&(bcb->mutex));

    // wait until there is an order, unless every order has already been handled
    while (IsEmpty(bcb)) {
        if (bcb->orders_handled >= bcb->expected_num_orders) {
            pthread_mutex_unlock(&(bcb->mutex));
            return NULL;
        }
        pthread_cond_wait(&(bcb->can_get_orders), &(bcb->mutex));
    }

    // take the order at the front of the queue
    order = bcb->orders;
    bcb->orders = order->next;
    bcb->current_size--;
    bcb->orders_handled++;

    // there is room for another order now
    pthread_cond_signal(&(bcb->can_add_orders));

    // if that was the last order, wake up the other cooks so they can go home
    if (bcb->orders_handled >= bcb->expected_num_orders) {
        pthread_cond_broadcast(&(bcb->can_get_orders));
    }

    pthread_mutex_unlock(&(bcb->mutex));
    return order;
}

// Optional helper functions (you can implement if you think they would be useful)
bool IsEmpty(BENSCHILLIBOWL* bcb) {
  return bcb->current_size == 0;
}

bool IsFull(BENSCHILLIBOWL* bcb) {
  return bcb->current_size == bcb->max_size;
}

/* this methods adds order to rear of queue */
void AddOrderToBack(Order **orders, Order *order) {
    if (*orders == NULL) {
        *orders = order;
    } else {
        Order *curr = *orders;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = order;
    }
}