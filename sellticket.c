#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

typedef struct passenger{
    char* name;
    char* flight;   /*  Points to the char* int flight struct, will be freed while freeing flights  */
    int priority;   // 0 for standard, 2 for ordinary economy, 3 for veteran, 5 for ordinary business 6 for diplomat. Wanted class can be extrapolated from that
    int purchased_seat_class;   //  0 for standard, 1 for economy, 2 for business ((priority+1)/3)
    struct passenger* next_in_queue;
    struct passenger* next_in_database;
}PASS;

typedef struct queue{
    PASS* front;
    PASS* rear;
    int size;
}QUEUE;


typedef struct flight{
    char* name;
    int standard_seats;
    int economy_seats;
    int business_seats;
    int is_closed;
    QUEUE* waiting_queue;
    QUEUE* sold;
    struct flight* prev;
}FLIGHT;


struct heads{
    PASS* first_passenger;
    PASS* last_passenger;
    FLIGHT* last_flight;
};


PASS* get_passenger(PASS* first, char* name){
    if (!name){
        return NULL;
    }
    PASS* passenger = first;
    while (passenger){
        if (!strcmp(passenger->name, name)){
            return passenger;
        }
        passenger = passenger->next_in_database;
    }
    return NULL;
}


FLIGHT* get_flight(FLIGHT* last, char* name){
    if (!name){
        return NULL;
    }
    FLIGHT* flight = last;
    while (flight){
        if (!strcmp(flight->name, name)){
            return flight;
        }
        flight = flight->prev;
    }
    return NULL;
}


char* copy_string(char* src_str){
    char* new_str = malloc(sizeof(char) * (strlen(src_str) + 1));
    strcpy(new_str, src_str);
    return new_str;
}


QUEUE* create_queue(){
    QUEUE* queue = malloc(sizeof(QUEUE));
    queue->size = 0;
    queue->rear = NULL;
    queue->front = NULL;
    return queue;
}


/*  add seats to the flight with the given name. If it does not exist create it. Return NULL if an error occured, the flight otherwise  */
FLIGHT* add_seat(struct heads* heads, char* flight_name, char* class, int quota){
    FLIGHT* flight = get_flight(heads->last_flight, flight_name);

    if (!flight){   /*  Creating the flight with the given name */
        flight = malloc(sizeof(FLIGHT));
        flight->name = copy_string(flight_name);
        flight->business_seats = 0;
        flight->economy_seats = 0;
        flight->standard_seats = 0;
        flight->is_closed = 0;
        flight->waiting_queue = create_queue();
        flight->sold = create_queue();

        flight->prev = heads->last_flight;
        heads->last_flight = flight;
    }

    if (!strcmp(class, "business")){
        flight->business_seats += quota;
    }else if (!strcmp(class, "economy")){
        flight->economy_seats += quota;
    }else if (!strcmp(class, "standard")){
        flight->standard_seats += quota;
    }else {
        return NULL;
    }

    return flight;

}


PASS* push(QUEUE* queue, PASS* passenger, int check_prioriy){
    PASS* current = queue->front;
    if (!queue->size){  /*  queue is empty  */
        queue->front = passenger;
        queue->rear = passenger;
        queue->size++;
        return passenger;
    }

    if (passenger->priority > queue->front->priority && check_prioriy){  /*  this must come to to front of the queue  */
        passenger->next_in_queue = queue->front;
        queue->front = passenger;
        queue->size++;
        return passenger;
    }

    /*  Decide which passenger must come before this one in the queue    */

    while (current->next_in_queue && (current->next_in_queue->priority >= passenger->priority || !check_prioriy)){
        current = current->next_in_queue;
    }
    passenger->next_in_queue = current->next_in_queue;
    current->next_in_queue = passenger;

    if (current == queue->rear){
        queue->rear = passenger;
    }

    queue->size++;
    return passenger;
}


PASS* pop(QUEUE* queue){
    PASS* passenger = queue->front;
    if (queue->size){
        queue->front = passenger->next_in_queue;
        passenger->next_in_queue = NULL;
        queue->size--;
        if (!queue->size){  /*  Last element was popped */
            queue->rear = NULL;
        }
    }
    return passenger;
}


PASS* add_passenger(struct heads* heads, char* passenger_name, char* flight_name, int priority){
    PASS* passenger = get_passenger(heads->first_passenger, passenger_name);
    FLIGHT* flight = get_flight(heads->last_flight, flight_name);

    if (!passenger_name || !flight_name || passenger || !flight || flight->is_closed){
        return NULL;
    }

    passenger = malloc(sizeof(PASS));
    passenger->name = copy_string(passenger_name);
    passenger->priority = priority;
    passenger->purchased_seat_class = -1;
    passenger->flight = flight->name;
    passenger->next_in_queue = NULL;

    passenger->next_in_database = NULL;
    if (heads->first_passenger == NULL){
        heads->first_passenger = passenger;
    } else {
        heads->last_passenger->next_in_database = passenger;
    }
    heads->last_passenger = passenger;


    push(flight->waiting_queue, passenger, 1);
    return passenger;
}


char* file_to_string(char* path){
    FILE* file = fopen(path, "r");
    int length = 0;
    char* string ;//= malloc(0);

    if (!file){
        return NULL;
    }
    int c = fgetc(file);

    while (c != EOF){
        length++;
        string = realloc(string, (length + 1) * sizeof(char));
        string[length-1] = (char) c;
        c = fgetc(file);
    }
    string[length] = '\0';

    fclose(file);

    return string;
}


int class_in_queue(QUEUE *queue, int class){
    int total = 0;
    PASS* passenger = queue->front;

    while (passenger){
        if ((passenger->priority + 1)/3 == class){
            total++;
        }
        passenger = passenger->next_in_queue;
    }
    return total;
}


int sold_in_queue(QUEUE *queue, int class){
    int total = 0;
    PASS* passenger = queue->front;

    while (passenger){
        if (passenger->purchased_seat_class == class){
            total++;
        }
        passenger = passenger->next_in_queue;
    }
    return total;
}


void sell_tickets(FLIGHT* flight){
    PASS* passenger = NULL;
    int i = 0;
    int waiting_count = flight->waiting_queue->size;
    int wanted_class = 2;
    int remaining_quotas[3] = {flight->standard_seats - sold_in_queue(flight->sold, 0), flight->economy_seats -
            sold_in_queue(flight->sold, 1), flight->business_seats -
            sold_in_queue(flight->sold, 2)};

    for (i = 0; i < waiting_count; ++i) {
        passenger = pop(flight->waiting_queue);
        wanted_class = (passenger->priority + 1)/3;

        /*  Can passenger buy from the class they want?  */
        if (remaining_quotas[wanted_class] > 0){
            passenger->purchased_seat_class = wanted_class;
            push(flight->sold, passenger, 0);
            remaining_quotas[wanted_class]--;
        } else {
            push(flight->waiting_queue, passenger, 0);
        }
    }

    /*  Selling standard seats to remaining passengers  */

    while (remaining_quotas[0]){
        passenger = pop(flight->waiting_queue);
        if (!passenger){
            break;
        }
        passenger->purchased_seat_class = 0;
        push(flight->sold, passenger, 0);
        remaining_quotas[0]--;

    }

}


int priority_from_string(char* class){
    if (!class){
        return -1;
    }
    if (!strcmp(class, "business")){
        return 5;
    } else if (!strcmp(class, "economy")){
        return 2;
    } else if (!strcmp(class, "standard")){
        return 0;
    } else{
        return -1;
    }
}


char* class_from_int(int class_int){
    if (class_int == 2){
        return "business";
    } else if (class_int == 1){
        return "economy";
    } else if (class_int == 0){
        return "standard";
    } else{
        return "none";
    }
}


void report(struct heads* heads, char* flight_name, FILE* output){
    FLIGHT*  flight = get_flight(heads->last_flight, flight_name);
    PASS* passenger;
    if (!flight){
        fprintf(output, "error\r\n");
        return;
    }


    int i=2;
    fprintf(output, "report %s\r\n", flight_name);


    for (; i >-1 ; --i) {
        fprintf(output, "%s %d\r\n", class_from_int(i), sold_in_queue(flight->sold, i));
        passenger = flight->sold->front;

        while (passenger){
            if (passenger->purchased_seat_class == i){
                fprintf(output, "%s\r\n", passenger->name);
            }
            passenger = passenger->next_in_queue;
        }
    }

    fprintf(output, "end of report %s\r\n", flight_name);
}


void process_line(struct heads* heads, char *line, FILE *output){
    char* flight_name;
    char* class_string;
    char* count;
    char* passenger_name;
    char* priority_string;
    int class_int;
    int total;
    PASS* passenger = NULL;
    FLIGHT* flight = NULL;
    char *string_part;
    long ret;

    char* order = strtok_r(line, " ", &line);

    if (!strcmp(order, "addseat")){
        flight_name = strtok_r(line, " ", &line);
        class_string = strtok_r(line, " ", &line);
        count = strtok_r(line, " ", &line);
        ret = strtol(count, &string_part, 10);

        if (!flight_name || !class_string || !count || strlen(string_part)){
            fprintf(output, "error\r\n");
            return;
        }

        flight = add_seat(heads, flight_name, class_string, (int)ret);
        if (!flight){
            fprintf(output, "error\r\n");
            return;
        }

        fprintf(output, "addseats %s %d %d %d\r\n", flight_name, flight->business_seats, flight->economy_seats, flight->standard_seats);

    }else if (!strcmp(order, "enqueue")){
        flight_name = strtok_r(line, " ", &line);
        class_string = strtok_r(line, " ", &line);
        passenger_name = strtok_r(line, " ", &line);
        priority_string = strtok_r(line, " ", &line);
        class_int = priority_from_string(class_string);

        if ((priority_string && strcmp(priority_string, "diplomat") && strcmp(priority_string, "veteran")) ||
            (!strcmp(class_string, "standard") && priority_string) ||
            (!strcmp(class_string, "economy") && priority_string && strcmp(priority_string, "veteran")) ||
            (!strcmp(class_string, "business") && priority_string && strcmp(priority_string, "diplomat")) || class_int == -1)
        {
            fprintf(output, "error\r\n");
            return;
        }

        if (priority_string){
            class_int += 1;
        }

        passenger = add_passenger(heads, passenger_name, flight_name, class_int);

        if (!passenger){
            fprintf(output, "error\r\n");
            return;
        }

        flight = get_flight(heads->last_flight, passenger->flight);
        total = class_in_queue(flight->waiting_queue, (class_int + 1) / 3);

        fprintf(output, "queue %s %s %s %d\r\n", flight_name, passenger_name, class_string, total);


    }else if (!strcmp(order, "sell")){
        flight_name = strtok_r(line, " ", &line);
        flight = get_flight(heads->last_flight, flight_name);
        if (!flight || flight->is_closed ){
            fprintf(output, "error\r\n");
            return;
        }

        sell_tickets(flight);

        fprintf(output, "sold %s %d %d %d\r\n", flight_name, sold_in_queue(flight->sold, 2), sold_in_queue(flight->sold, 1),
                sold_in_queue(flight->sold, 0));

    }else if (!strcmp(order, "close")){
        flight_name = strtok_r(line, " ", &line);
        flight = get_flight(heads->last_flight, flight_name);

        if (!flight || flight->is_closed){
            fprintf(output, "error\r\n");
            return;
        }
        flight->is_closed = 1;
        fprintf(output, "closed %s %d %d\r\n", flight_name, flight->sold->size, flight->waiting_queue->size);


        passenger = pop(flight->waiting_queue);

        while (passenger){
            fprintf(output, "waiting %s\r\n", passenger->name);
            passenger = pop(flight->waiting_queue);
        }


    }else if (!strcmp(order, "report")){
        flight_name = strtok_r(line, " ", &line);
        report(heads, flight_name, output);

    }else if (!strcmp(order, "info")){
        passenger_name = strtok_r(line, " ", &line);
        passenger = get_passenger(heads->first_passenger, passenger_name);

        if (!passenger){
            fprintf(output, "error\r\n");
            return;
        }

        fprintf(output, "info %s %s %s %s\r\n", passenger_name, passenger->flight,
                class_from_int((passenger->priority + 1) / 3),
                class_from_int(passenger->purchased_seat_class));

    }else{
        fprintf(output, "error\r\n");
    }

}


void free_flights_and_passengers(struct heads* heads){
    PASS* current_passenger = heads->first_passenger;
    FLIGHT* current_flight = heads->last_flight;
    PASS* next_passenger;
    FLIGHT* next_flight;

    while (current_passenger){
        next_passenger = current_passenger->next_in_database;
        free(current_passenger->name);
        free(current_passenger);
        current_passenger = next_passenger;
    }

    while (current_flight){
        next_flight = current_flight->prev;
        free(current_flight->name);
        free(current_flight->waiting_queue);
        free(current_flight->sold);
        free(current_flight);
        current_flight = next_flight;
    }

}


int main(int argc, char* args[]) {
    char* input_lines = file_to_string(args[1]);
    char* rest = input_lines;
    FILE* output = fopen(args[2], "w+");

    struct heads heads;
    heads.first_passenger = NULL;
    heads.last_flight = NULL;
    heads.last_passenger = NULL;

    char* line = NULL;

    while ((line = strtok_r(rest, "\r\n", &rest))){
        process_line(&heads, line, output);
    }

    fclose(output);
    free(input_lines);
    free_flights_and_passengers(&heads);

    return 0;
}