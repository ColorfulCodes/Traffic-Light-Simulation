#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include <applibs/log.h>
#include <applibs/gpio.h>

// Good for type checking
#define LED_STATE enum {GREEN_STATE = 1, YELLOW_STATE, RED_STATE}
const int BUTTON_PRESSED = 0;
const int BUTTON_UNPRESSED = 1;

//This time spec is in nano seconds but equates to 10ms
const struct timespec sleepTime = { 0, 10000000 };
const struct timespec oneSec = { 1, 0 };
const struct timespec threeSec = { 3, 0 };
LED_STATE active_led = GREEN_STATE;
int printCrossWalk;



//Declare function
int traffic_light(int, int, int, int, int, int, int);


//Build the LL
struct node {
    struct node* next; // pointer of structure type
    char value[];
};

// set existing type, node, to the alias, node_t
typedef struct node node_t;

node_t* tmp1;
// declaring head pointer
node_t* head1 = NULL;


node_t* create_new_node(const char* value)
{
    // create space for node with malloc
    node_t* result = malloc(sizeof(result) + strlen(value) + 1);
    if (result)
    {
        strcpy(result->value, value);
        result->next = NULL;
    }
    return result;
}

node_t* insert_at_head(node_t** head, node_t* node_to_insert) {

    node_to_insert->next = *head;
    *head = node_to_insert;
    return node_to_insert;
}

void delete_node(node_t* head) {
    // move through the head
    node_t* current = head;
    node_t* prev = NULL;
    while (current->next != NULL) {
        prev = current;
        current = current->next;
    }
    prev->next = current->next;
    prev = current;

    // To free malloc space
    free(current);
}

//Get the Length of the Linked List storage
int get_length(node_t* head) {
    node_t* current = head;
    int count = 0;
    while (current != NULL) {
        current = current->next;
        count += 1;
    }
    return count;
}


//Prints linked list
void printlist(node_t* head) {
    node_t* temporary = head;
    int count = 1;

    while (temporary != NULL) {

        //print out the value of the node that temporary points to
        Log_Debug("%d. %s\n", count, temporary->value);
        // to move along the list
        temporary = temporary->next;
        count++;
    }
    Log_Debug("\n");
    exit(0);
}

void limit_reached(void) {
    //Call get length function, if true continue, else call delete node function
    while (get_length(head1) > 5) {
        //delete node as space is full
        delete_node(head1);
    }
    printlist(head1);
}


GPIO_Value_Type previous_a_value = BUTTON_UNPRESSED;
GPIO_Value_Type previous_b_value = BUTTON_UNPRESSED;

int WakeableWait(int y1, int y2, int btnA, int green_led, int red_led, int blue_led, int btnB, int milliseconds) {
    Log_Debug("In Wake\n");

    while (milliseconds > 0) {
        // Return true if full time elapsed, false if button was pressed
        GPIO_Value_Type value;
        GPIO_GetValue(btnA, &value);

        GPIO_Value_Type value2;
        GPIO_GetValue(btnB, &value2);

        bool button_a_pressed = (previous_a_value == BUTTON_UNPRESSED) && (value == BUTTON_PRESSED);
        previous_a_value = value;

        bool button_b_pressed = (previous_b_value == BUTTON_UNPRESSED) && (value2 == BUTTON_PRESSED);
        previous_b_value = value2;

        if (button_a_pressed) {
            //Switch to Traffic Light
            return 3;
        }

        if (button_b_pressed) {
            //Print out logs
            return 4;

        }

        nanosleep(&sleepTime, NULL);
        milliseconds -= 10;

    }return 1;

}

// This function should blink Yellow until button A is pressed to switch to typical working traffic light
void caution_light(int y1, int y2, int btnA, int green_led, int red_led, int blue_led, int button_B) {


    //// This should blink on and off for one second
    while (true) {
        // Turn on Yellow
        GPIO_SetValue(y1, GPIO_Value_Low);
        GPIO_SetValue(y2, GPIO_Value_Low);

        printCrossWalk = WakeableWait(y1, y2, btnA, green_led, red_led, blue_led, button_B, 1000);
        // why does it return 1 instead of 4

        if (printCrossWalk == 4) {
            limit_reached();
        }

//         Switch to traffic led
        if (printCrossWalk == 3) {
            traffic_light(y1, y2, btnA, green_led, red_led, blue_led, button_B);;
        }
        GPIO_SetValue(y1, GPIO_Value_High);
        GPIO_SetValue(y2, GPIO_Value_High);

        printCrossWalk = WakeableWait(y1, y2, btnA, green_led, red_led, blue_led, button_B, 1000);

        if (printCrossWalk == 4) {
            limit_reached();
        }

        if (printCrossWalk == 3) {
            traffic_light(y1, y2, btnA, green_led, red_led, blue_led, button_B);;
        }

    }
}

int traffic_light(int y1, int y2, int btnA, int green_led, int red_led, int blue_led, int btnB) {
    // turn off after cross walk
    GPIO_SetValue(red_led, GPIO_Value_High);
    GPIO_SetValue(blue_led, GPIO_Value_High);

    // place a sleep here so that the caution led transition has time to stay into traffic state
    nanosleep(&oneSec, NULL);
    int timer_countdown = 200;
    // check to see if timer is finished
    bool time_is_finished = false;


    while (true) {

        // check if button A is pressed
        GPIO_Value_Type value;
        GPIO_GetValue(btnA, &value);

        // check if button B is pressed
        GPIO_Value_Type value2;
        GPIO_GetValue(btnB, &value2);

        if (timer_countdown > 0) {
            --timer_countdown;
            // set to true or false
            time_is_finished = (timer_countdown == 0);
        }

        if (value2 == BUTTON_PRESSED) {
            time_t rawtime;
            struct tm* grabtime;
            time(&rawtime);
            grabtime = localtime(&rawtime);

            // Creating and adding time to LINKED LIST
            tmp1 = create_new_node(asctime(grabtime));
            insert_at_head(&head1, tmp1);

            // Turn off green and red leds
            GPIO_SetValue(green_led, GPIO_Value_High);
            GPIO_SetValue(red_led, GPIO_Value_High);

            //count down yellow 2 seconds
            //Why not working? Only works in debugger
            GPIO_SetValue(y1, GPIO_Value_Low);
            GPIO_SetValue(y2, GPIO_Value_Low);
            WakeableWait(y1, y2, btnA, green_led, red_led, blue_led, btnB, 2000);

            // turn off yellow leds
            GPIO_SetValue(y1, GPIO_Value_High);
            GPIO_SetValue(y2, GPIO_Value_High);

            // count down crosswalk 3 seconds: blue and red leds
            // Turn on Red Led
            GPIO_SetValue(red_led, GPIO_Value_Low);

            //Turn on Blue led to signify cross walk
            GPIO_SetValue(blue_led, GPIO_Value_Low);

            // Wait for 3 seconds then restart traffic light
            WakeableWait(y1, y2, btnA, green_led, red_led, blue_led, btnB, 3000);

            // Turn off blue and red led when traffic called from crosswalk
            GPIO_SetValue(red_led, GPIO_Value_High);
            GPIO_SetValue(blue_led, GPIO_Value_High);

            // Return to normal traffic
            active_led = GREEN_STATE;
            traffic_light(y1, y2, btnA, green_led, red_led, blue_led, btnB);
        }

        if (value == BUTTON_PRESSED) {
            // turn off other led's when switching back to caution state
            GPIO_SetValue(green_led, GPIO_Value_High);
            GPIO_SetValue(red_led, GPIO_Value_High);
            // return to caution state
            return 0;
        }

        if (active_led == GREEN_STATE) {
            Log_Debug("Currently Green\n");
            // Turn on green led
            GPIO_SetValue(green_led, GPIO_Value_Low);
            // Turn off other leds
            GPIO_SetValue(red_led, GPIO_Value_High);
            GPIO_SetValue(y1, GPIO_Value_High);
            GPIO_SetValue(y2, GPIO_Value_High);

            if (time_is_finished == true) {
                active_led = YELLOW_STATE;
                timer_countdown = 50;
            }
        }

        else if (active_led == YELLOW_STATE) {
            Log_Debug("Currently Yellow\n");
            // turn on yellow
            GPIO_SetValue(y1, GPIO_Value_Low);
            GPIO_SetValue(y2, GPIO_Value_Low);
            // turn off other leds
            GPIO_SetValue(red_led, GPIO_Value_High);
            GPIO_SetValue(green_led, GPIO_Value_High);

            if (time_is_finished == true) {
                active_led = RED_STATE;
                timer_countdown = 200;
            }

        }
        else if (active_led == RED_STATE) {
            Log_Debug("Currently Red\n");
            // turn on red
            GPIO_SetValue(red_led, GPIO_Value_Low);
            // turn off other leds
            GPIO_SetValue(green_led, GPIO_Value_High);
            GPIO_SetValue(y1, GPIO_Value_High);
            GPIO_SetValue(y2, GPIO_Value_High);

            if (time_is_finished == true) {
                active_led = GREEN_STATE;
                timer_countdown = 200;
            }
        }

        nanosleep(&sleepTime, NULL);
    }
}


int main(void)
{
    int green_led = GPIO_OpenAsOutput(9, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (green_led < 0) {
        Log_Debug(
            "Error opening GPIO: %s (%d). Check that app_manifest.json includes the GPIO used.\n",
            strerror(errno), errno);
        return -1;
    }

    int red_led = GPIO_OpenAsOutput(18, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (red_led < 0) {
        Log_Debug(
            "Error opening GPIO: %s (%d). Check that app_manifest.json includes the GPIO used.\n",
            strerror(errno), errno);
        return -1;
    }

    int blue_led = GPIO_OpenAsOutput(10, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (blue_led < 0) {
        Log_Debug(
            "Error opening GPIO: %s (%d). Check that app_manifest.json includes the GPIO used.\n",
            strerror(errno), errno);
        return -1;
    }

    int yellow_led = GPIO_OpenAsOutput(15, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (yellow_led < 0) {
        Log_Debug(
            "Error opening GPIO: %s (%d). Check that app_manifest.json includes the GPIO used.\n",
            strerror(errno), errno);
        return -1;
    }

    int yellow_led2 = GPIO_OpenAsOutput(16, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (yellow_led2 < 0) {
        Log_Debug(
            "Error opening GPIO: %s (%d). Check that app_manifest.json includes the GPIO used.\n",
            strerror(errno), errno);
        return -1;
    }

    int button_A = GPIO_OpenAsInput(12);
    if (button_A < 0) {
        Log_Debug(
            "Error opening GPIO: %s (%d). Check that app_manifest.json includes the GPIO used.\n",
            strerror(errno), errno);
        return -1;
    }

    int button_B = GPIO_OpenAsInput(13);
    if (button_B < 0) {
        Log_Debug(
            "Error opening GPIO: %s (%d). Check that app_manifest.json includes the GPIO used.\n",
            strerror(errno), errno);
        return -1;
    }
    //activate caution blink first
    // pass in light variables
    caution_light(yellow_led, yellow_led2, button_A, green_led, red_led, blue_led, button_B);

    return 0;


}
