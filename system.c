/* Alaa Zuhd - 1180865
Rawan Yassin - 1182224
*/
#include "local.c"

//some needed constants:
#define MAX_BOX_CAPACITY 10
#define NUMBER_OF_TRUCKS 3
#define NUMBER_OF_LINES 10
#define NUMBER_OF_TECHNICAL_EMPLOYEES_PER_LINE 10  //Fixed
#define NUMBER_OF_STORAGE_EMPLOYEES_PER_LINE 1
#define NUMBER_OF_LOADING_EMPLOYESS 5
#define NUMBER_OF_STORAGE_AREA_KEEPERS 1

//the values of these global variable will be user-defined and passed by the configuration.txt file
int storage_employee_processing_time;
int max_storage_area_threshold;
int min_storage_area_threshold;
int truck_capacity;
int truck_processing_time;
int salary_CEO;
int salary_HR;
int salary_technical_employee;
int salary_storage_employee;
int salary_loading_employee;
int salary_driver_employee;
int salary_storage_area_keeper;
int laptop_cost;
int laptop_price;
int max_gains_threshold;
int min_profit_threshold;
int accepted_profit_threshold;
int max_technical_employee_processing_time;
int min_technical_employee_processing_time;

//functions prototypes
void read_configuration(char *);
void steps_one_to_five(void *);         //used by technical employees performing steps from 1-5
void steps_six_to_ten(void *);          //used by technical employees performing steps from 6-10
void collect_full_box(int *);           //used by storage employees
void load_trucks();                     //used by loading employees 
void truck_function(int *);             //used by trucks
void storage_area_controller_function();//used by storage-area-keeper employee
void CEO_function();                    //used by CEO employee
void HR_function();                     //used by HR employee
void initialize_cond_mutex();     
void destroy_cond_mutex();
void create_threads();
void cancel_threads();

//keep track of the current number of ready-manufactured laptops in a cartoon box, one box at a time, for each seperate line
int in_box[NUMBER_OF_LINES] = {0};
pthread_mutex_t in_box_mutex[NUMBER_OF_LINES];
pthread_cond_t in_box_cond[NUMBER_OF_LINES];

//used to control when each line would work and when it is suspended
int suspend_flag[NUMBER_OF_LINES] = {0};
pthread_mutex_t suspend_flag_mutex[NUMBER_OF_LINES];
pthread_cond_t suspend_flag_cond[NUMBER_OF_LINES];

//used to control the capacity of the storage area
int in_storage_area = 0;
pthread_mutex_t in_storage_area_mutex;
pthread_cond_t in_storage_area_cond;

//controlling the CEO_HR communication
int CEO_HR = 0;
pthread_mutex_t CEO_HR_mutex;
pthread_cond_t CEO_HR_cond;

//controlling the simulation, and deciding when it should end
pthread_mutex_t simulation_mutex;
pthread_cond_t simulation_cond;
int total_number_of_technical_employees; 
int total_number_of_storage_employees;
int counter[NUMBER_OF_LINES] = {0};
int ready_laptops_counter[NUMBER_OF_LINES] = {0};

//controlling the trucks 
int truck_current_capacity[NUMBER_OF_TRUCKS] = {0};
pthread_mutex_t truck_mutex[NUMBER_OF_TRUCKS];

//to ensure that each laptop is in one step 
pthread_mutex_t laptops_mutex[NUMBER_OF_LINES][NUMBER_OF_TECHNICAL_EMPLOYEES_PER_LINE];

//a queue of laptops for each line
struct Queue* laptops[NUMBER_OF_LINES];

//threads-ids 
pthread_t technical_employees[NUMBER_OF_LINES][NUMBER_OF_TECHNICAL_EMPLOYEES_PER_LINE];
pthread_t storage_employee[NUMBER_OF_LINES];  
pthread_t loading_employees[NUMBER_OF_LOADING_EMPLOYESS];
pthread_t truck[NUMBER_OF_TRUCKS]; 
pthread_t storage_area_controller;
pthread_t CEO;
pthread_t HR;

int main(int argc, char *argv[]){  
    //checking the number of passed arguments
    if(argc != 2) {
        perror("Number of arguments must be 2!");
        exit(-1);
    } 
    printf("\033[1;34m"); // set the color to green
    printf("\t \t \t Welcome To Our Samrt Factory!\n");
    printf("\t \t \t ------------------------------\n");
    printf("\033[0m");// reset the color to the default
    //read the configuration file
    read_configuration(argv[1]); 
    //at first, the total number of technical and storage employees is as follows:
    total_number_of_technical_employees = NUMBER_OF_LINES * NUMBER_OF_TECHNICAL_EMPLOYEES_PER_LINE;
    total_number_of_storage_employees = NUMBER_OF_LINES * NUMBER_OF_STORAGE_EMPLOYEES_PER_LINE;
    //for random generation puposes
    srand((unsigned int)getpid());
    //getting the mutex and condional variables ready
    initialize_cond_mutex();
    //Creating the Queues, each line is made of ten queues- one queue per technical employee
    for(int line=0;line<NUMBER_OF_LINES;line++){
        laptops[line] = createQueue();
        for(int i=0; i<NUMBER_OF_TECHNICAL_EMPLOYEES_PER_LINE; i++){
            laptops[line]->step[i] = createQueue();
        }
    }
    //starting the program, after having all the resources ready
    create_threads();
    //Controllong the end of the simulation
    pthread_mutex_lock(&simulation_mutex);
    pthread_cond_wait(&simulation_cond, &simulation_mutex);
    pthread_mutex_unlock(&simulation_mutex);
    //if either the gains are greater than the passed maximum gain, or the number of suspended lines is more than 50%, simulation ends: 
    //all created threads need to be cancelled so that mutexes and conditonal variables can be destroyed
    cancel_threads();
    //get rid of all created mutes and conditional varaibles 
    destroy_cond_mutex();
    printf("\033[5;34m"); // set the color to green
    printf("Simulation Ends\n");
    printf("\033[0m");// reset the color to the default
    exit(0);  
}  
//this function is used to cancel all created threads, using the cancel function, cancellation is needed in order to get rid of the created mutex and conditional variables.
void cancel_threads(){
    for (int line = 0; line < NUMBER_OF_LINES; line++) {
        pthread_cancel(storage_employee[line]);
        for(int step=0; step<NUMBER_OF_TECHNICAL_EMPLOYEES_PER_LINE; step++)
            pthread_cancel(technical_employees[line][step]);   
    }   
    for(int i=0; i<NUMBER_OF_LOADING_EMPLOYESS;i++){
        pthread_cancel(loading_employees[i]);
    }
    for(int i=0; i<NUMBER_OF_TRUCKS; i++) {
        pthread_cancel(truck[i]);
    }
    pthread_cancel(storage_area_controller);
    pthread_cancel(CEO);
    pthread_cancel(HR);
}
//this function is used to read all the user-defined variables from a file and parsing them
void read_configuration(char *file_name) {
    char read[100];
    FILE *fptr;
    fptr = fopen(file_name,"r"); 
    if (fptr == NULL){
        printf("\033[0;31m"); // set the color to red 
        perror("Error while opening the file.\n");
        printf("\033[0m");// reset the color to the default
        exit(-1);
    }
    while (fgets(read, sizeof(read), fptr)) {
        char *token = strtok(read," ");
        if(strcmp(token, "STORAGE_EMPLOYEE_PROCESSING_TIME") == 0){
            storage_employee_processing_time = atoi(strtok(NULL,"\n"));
        }else if (strcmp(token, "MAX_TECHNICAL_EMPLOYEE_PROCESSING_TIME") == 0){
            max_technical_employee_processing_time = atoi(strtok(NULL,"\n"));
        }else if (strcmp(token, "MIN_TECHNICAL_EMPLOYEE_PROCESSING_TIME") == 0){
            min_technical_employee_processing_time = atoi(strtok(NULL,"\n"));
        }else if (strcmp(token, "MAX_STORAGE_AREA_THRESHOLD") == 0){
            max_storage_area_threshold = atoi(strtok(NULL,"\n"));
        }else if (strcmp(token, "MIN_STORAGE_AREA_THRESHOLD") == 0){
            min_storage_area_threshold = atoi(strtok(NULL,"\n"));
        }else if (strcmp(token, "TRUCK_CAPACITY" ) == 0){
            truck_capacity = atoi(strtok(NULL,"\n"));
            truck_capacity = truck_capacity * MAX_BOX_CAPACITY;
        }else if (strcmp(token, "TRUCK_PROCESSING_TIME") == 0){
            truck_processing_time = atoi(strtok(NULL,"\n"));
        }else if (strcmp(token, "SALARY_CEO") == 0){
            salary_CEO = atoi(strtok(NULL,"\n"));
        }else if (strcmp(token, "SALARY_HR" ) == 0){
            salary_HR = atoi(strtok(NULL,"\n"));
        }else if (strcmp(token, "SALARY_TECHNICAL_EMPLOYEE" ) == 0){
            salary_technical_employee = atoi(strtok(NULL,"\n"));
        }else if (strcmp(token, "SALARY_STORAGE_EMPLOYEE" ) == 0){
            salary_storage_employee = atoi(strtok(NULL,"\n"));
        } else if (strcmp(token, "SALARY_LOADING_EMPLOYEE" ) == 0){
            salary_loading_employee = atoi(strtok(NULL,"\n"));
        } else if (strcmp(token, "SALARY_DRIVER_EMPLOYEE" ) == 0){
            salary_driver_employee = atoi(strtok(NULL,"\n"));
        } else if (strcmp(token, "SALARY_STORAGE_AREA_KEEPER" ) == 0){
            salary_storage_area_keeper = atoi(strtok(NULL,"\n"));
        }else if (strcmp(token, "LAPTOP_COST" ) == 0){
            laptop_cost = atoi(strtok(NULL,"\n"));
        } else if (strcmp(token, "LAPTOP_PRICE" ) == 0){
            laptop_price = atoi(strtok(NULL,"\n"));
        } else if (strcmp(token, "MIN_PROFIT_THRESHOLD" ) == 0){
            min_profit_threshold = atoi(strtok(NULL,"\n"));
        } else if (strcmp(token, "ACCEPTED_PROFIT_THRESHOLD" ) == 0){
            accepted_profit_threshold = atoi(strtok(NULL,"\n"));
        } else if (strcmp(token, "MAX_FACTORY_GAINS_THRESHOLD" ) == 0){
            max_gains_threshold = atoi(strtok(NULL,"\n"));
        } 
    }
    if(fclose(fptr)) {
        exit(-1);
    }
}
//this function is used to get all mutex and condtional variables ready ( getting the resources ready)
void initialize_cond_mutex(){
    for(int line=0;line<NUMBER_OF_LINES;line++){
        for(int step=0; step<NUMBER_OF_TECHNICAL_EMPLOYEES_PER_LINE; step++) {
            pthread_mutex_init(&laptops_mutex[line][step],NULL);
        }
    }
    for(int line=0; line<NUMBER_OF_LINES; line++){
        pthread_mutex_init(&in_box_mutex[line],NULL);
        pthread_cond_init(&in_box_cond[line],NULL);
        pthread_mutex_init(&suspend_flag_mutex[line],NULL);
        pthread_cond_init(&suspend_flag_cond[line],NULL); 
    }
    for(int i=0; i<NUMBER_OF_TRUCKS; i++){
         pthread_mutex_init(&truck_mutex[i],NULL);
    }
    pthread_mutex_init(&in_storage_area_mutex,NULL);
    pthread_cond_init(&in_storage_area_cond,NULL);
    pthread_mutex_init(&CEO_HR_mutex,NULL);
    pthread_cond_init(&CEO_HR_cond,NULL);
    pthread_mutex_init(&simulation_mutex,NULL);
    pthread_cond_init(&simulation_cond,NULL);
}
//this function is used to destroy all created mutex and conditional variables( freeing the used resources)
void destroy_cond_mutex() { 
    for(int line=0; line<NUMBER_OF_LINES; line++){
        pthread_mutex_destroy(&in_box_mutex[line]);
        pthread_cond_destroy(&in_box_cond[line]);
        pthread_mutex_destroy(&suspend_flag_mutex[line]);
        pthread_cond_destroy(&suspend_flag_cond[line]); 
    }
    for(int i=0; i<NUMBER_OF_TRUCKS; i++){
        pthread_mutex_destroy(&truck_mutex[i]);
    }
    for(int line=0; line<NUMBER_OF_LINES; line++){
        for(int step=0; step<NUMBER_OF_TECHNICAL_EMPLOYEES_PER_LINE; step++){
            pthread_mutex_destroy(&laptops_mutex[line][step]);
        }
    }
    pthread_mutex_destroy(&in_storage_area_mutex);
    pthread_cond_destroy(&in_storage_area_cond);
    pthread_mutex_destroy(&CEO_HR_mutex);
    pthread_cond_destroy(&CEO_HR_cond);
    pthread_mutex_destroy(&simulation_mutex);
    pthread_cond_destroy(&simulation_cond);
}
//this function is responsible of creating all the needed threads for the all the steps of the factory
void create_threads() {
    for(int i=0; i<NUMBER_OF_LOADING_EMPLOYESS;i++){ 
        pthread_create(&loading_employees[i],NULL,load_trucks, (void *)i);
    }
    for(int i=0; i<NUMBER_OF_TRUCKS;i++){
        pthread_create(&truck[i],NULL,truck_function, (void *)i);
    }
    pthread_create(&storage_area_controller,NULL, storage_area_controller_function, NULL);
    for(int line=0; line<NUMBER_OF_LINES; line++) {
        pthread_create(&storage_employee[line],NULL, collect_full_box, (void *)line );  
        //creating the first five technical employees of each line, these employees must work one after the another, with pipelining approach
        for(int i=0; i<5; i++) {
            struct thread_args *args = malloc (sizeof (struct thread_args));
            args->line_num = line;
            args->step_num = i; 
            pthread_create(&technical_employees[line][i],NULL,steps_one_to_five, (void *)args);
        }
        //creating the rest of the technical employees of each line, can work in any order 
        for(int i=5; i<10; i++) {
            struct thread_args *args = malloc (sizeof (struct thread_args));
            args->line_num = line;
            args->step_num = i; 
            pthread_create(&technical_employees[line][i],NULL,steps_six_to_ten, (void *)args);
        }
    }
    pthread_create(&CEO,NULL, CEO_function, NULL);
    pthread_create(&HR,NULL, HR_function, NULL);
}
//this function handles the first five steps of each line as follows: 
void steps_one_to_five(void *_args) {
    //for each thread, the line number as well as this technical employee number (number represents the step this employee is performing) is passed to the function 
    struct thread_args *args = (struct thread_args *) _args;
    int step_num = args->step_num; 
    int line_num = args->line_num; 
    while(1) {
        sleep(1);
        //Getting the needed mutexes
        if(step_num == 0){             //if I am step 0, then I will create a laptop, and place it in the technical employee's 2 queue of the same line
            pthread_mutex_lock(&suspend_flag_mutex[line_num]);
            if(suspend_flag[line_num] == 0) {
                pthread_mutex_unlock(&suspend_flag_mutex[line_num]);
                //creating the laptop
                struct laptop* temp = newNode(line_num,step_num,counter[line_num]);
                //increment the counter of this line
                counter[line_num]++;  
                //enqueue in the right next queue
                enQueue(laptops[line_num]->step[step_num+1],temp->counter,temp->step[0],temp->step[1],temp->step[2],temp->step[3],temp->step[4],temp->step[5],temp->step[6],temp->step[7],temp->step[8],temp->step[9],temp->counter);
                sleep(rand()%max_technical_employee_processing_time + min_technical_employee_processing_time);
                printf("Line NO.%d |   TECH_EMP NO.%d, done working on laptop NO.%d, laptop now will move to TECH_EMP NO.%d\n", line_num,step_num,temp->counter,step_num+1);
                sleep(2);
            } else {
                pthread_mutex_unlock(&suspend_flag_mutex[line_num]);
            } 
        }else {
            //if I am any step 1,2,3, I should dequeue the laptop, perfrom my step, and enqueue in the next technical employee's queue. 
            //if I am step 4, I will enqueue in a random employee between 5 to 10 
            struct laptop* temp = deQueue(laptops[line_num]->step[step_num]);
            if( temp!=NULL){ //make sure that the queue is not empty
                if(temp->step[step_num]==0){
                    temp->step[step_num]=1;
                    if(step_num == 4) { 
                        int random_number = rand()%5 + 5; 
                        sleep(rand()%max_technical_employee_processing_time + min_technical_employee_processing_time);
                        printf("Line NO.%d |   TECH_EMP NO.%d, done working on laptop NO.%d, laptop now will move to TECH_EMP NO.%d\n", line_num, step_num,temp->counter, random_number);
                        struct laptop* new_temp = temp;
                        enQueue(laptops[line_num]->step[random_number],temp->counter,temp->step[0],temp->step[1],temp->step[2],temp->step[3],temp->step[4],temp->step[5],temp->step[6],temp->step[7],temp->step[8],temp->step[9],temp->counter);
                    } else { 
                        sleep(rand()%max_technical_employee_processing_time + min_technical_employee_processing_time);
                        printf("Line NO.%d |   TECH_EMP NO.%d, done working on laptop NO.%d, laptop now will move to TECH_EMP NO.%d\n", line_num, step_num,temp->counter, step_num+1);
                        enQueue(laptops[line_num]->step[step_num+1],temp->counter,temp->step[0],temp->step[1],temp->step[2],temp->step[3],temp->step[4],temp->step[5],temp->step[6],temp->step[7],temp->step[8],temp->step[9],temp->counter);
                    }
                }
            }
            else{
                // queue is empty 
                sleep(2);
            }
        }
    }
}

//this funtion is used by technical employees performing steps between 6 to 10, these steps can happen in any order 
void steps_six_to_ten(void *_args) {
    //for each thread, the line number as well as this technical employee number (number represents the step this employee is performing) is passed to the function 
    struct thread_args *args = (struct thread_args *) _args;
    int step_num = args->step_num; 
    int line_num = args->line_num; 
    while(1) {
        sleep(1);
        //get the needed mutexes
        struct laptop* temp2 = deQueue(laptops[line_num]->step[step_num]);
        if( temp2!=NULL){ //there is a loptop in this technical employee queue
            if( temp2->step[step_num] == 0){            //check that this step is not performed in this laptop, if yes enter to do the work
                sleep(rand()%max_technical_employee_processing_time + min_technical_employee_processing_time);
                temp2->step[step_num]=1;                //set this step as done for this laptop
                int counter = 0;                        //check that there are still steps to be performed in this laptop
                for(int i=5; i<10; i++){
                    if(temp2->step[i] == 1){
                        counter ++;
                    }      
                }
                if(counter < 5){                        //if there are still stpes to be performed in this laptopn
                    int random_number = rand()%5 + 5;   //generate a random number of the next step to be performed, and make sure that it is not performed on this laptop yet
                    while (temp2->step[random_number]!=0)
                    {
                        random_number = rand()%5 + 5;    
                    }
                    printf("Line NO.%d |   TECH_EMP NO.%d, done working on laptop NO.%d, laptop now will move to TECH_EMP NO.%d\n", line_num, step_num,temp2->counter, random_number);
                    enQueue(laptops[line_num]->step[random_number],temp2->counter,temp2->step[0],temp2->step[1],temp2->step[2],temp2->step[3],temp2->step[4],temp2->step[5],temp2->step[6],temp2->step[7],temp2->step[8],temp2->step[9],temp2->counter);
                }
                else if(counter == 5) {                 //if all steps are performed, then this last technical perforing this step, should put the laptop in a box
                    //getting the box in this specific line mutex
                    pthread_mutex_lock(&in_box_mutex[line_num]);
                    //incrementing the number of laptops in this box
                    in_box[line_num] ++; 
                    //incrementing the number of ready laptops
                    ready_laptops_counter[line_num]++;
                    printf("Line NO.%d |   Laptop NO.%d is ready and now in the box added by TECH_EMP NO.%d.\n", line_num, temp2->counter, step_num);
                    pthread_cond_signal(&in_box_cond[line_num]);
                    pthread_mutex_unlock(&in_box_mutex[line_num]);
                } 
            }
        }
    }
}

//this function is used by the storage employee, to move ready boxes( reaching the maximum number of laptops in a box) and moving them to the storage area
void collect_full_box(int *arg) {
     //for each thread, the line number is passed
    int line_num = (int) arg; 
    while(1) {
        //getting the needed mutex to read the box value
        pthread_mutex_lock(&in_box_mutex[line_num]);
        //keep waiting, move the box when it is full
        while(in_box[line_num] < MAX_BOX_CAPACITY) { 
            pthread_cond_wait(&in_box_cond[line_num], &in_box_mutex[line_num]);
        } 
        in_box[line_num] = 0; 
        //getting the storage area mutex to store on it 
        pthread_mutex_lock(&in_storage_area_mutex);
        in_storage_area +=MAX_BOX_CAPACITY; 
        printf("Line NO.%d |   One box is ready and stored in the storage area - Current Storage area: %d\n", line_num, in_storage_area);
        sleep(storage_employee_processing_time);
        //signaling the storage area keeper that a change is made on the storage area
        pthread_cond_signal(&in_storage_area_cond);
        //releasing the mutexes
        pthread_mutex_unlock(&in_storage_area_mutex); 
        pthread_mutex_unlock(&in_box_mutex[line_num]);
    }
}
//this function is used by the loading employees to load the boxes, from the storage area to the trucks
void load_trucks(){
    while(1){
        //get the storage area mutex, as its value is to be edited
        pthread_mutex_lock(&in_storage_area_mutex);
        //if there is an available box in the area, transfer it
        if(in_storage_area >= MAX_BOX_CAPACITY ){
            for(int i=0; i<NUMBER_OF_TRUCKS; i++){
                if(pthread_mutex_trylock(&truck_mutex[i])==0){                            //get the mutex of the available truck
                    if(truck_current_capacity[i]+MAX_BOX_CAPACITY <= truck_capacity){ //can the truck be loaded by more cartons
                        truck_current_capacity[i] += MAX_BOX_CAPACITY;
                        in_storage_area -= MAX_BOX_CAPACITY;
                        pthread_cond_signal(&in_storage_area_cond);                       //signaling the storage area that a change has been made
                        printf("One box is loaded into truck NO.%d - Current Storage area: %d\n", i, in_storage_area);
                        pthread_mutex_unlock(&truck_mutex[i]);
                        break;
                    }
                    pthread_mutex_unlock(&truck_mutex[i]);
                }
            }  
        }
        pthread_mutex_unlock(&in_storage_area_mutex);
    }
}
//this function is used by the atorage are controller to periodically check that the storage area is in the needed range
void storage_area_controller_function(){
    while(1){
        pthread_mutex_lock(&in_storage_area_mutex);
        //keep checking the storage area, until it is full 
        while(in_storage_area < max_storage_area_threshold) { 
            pthread_cond_wait(&in_storage_area_cond, &in_storage_area_mutex);
        }
        printf("\033[5;31m"); // set the color to red 
        printf("#################### Storage Area is full, need to suspend all lines in the factory. ####################\n");
        printf("\033[0m");// reset the color to the default
        //when it is full, suspend all lines 
        for(int line=0; line<NUMBER_OF_LINES; line++) { // suspend all lines 
            pthread_mutex_lock(&suspend_flag_mutex[line]);
            suspend_flag[line] = 1; 
            pthread_mutex_unlock(&suspend_flag_mutex[line]);
        }
        //keep checking the storage area, until it is below the given minimum threshold
        while(in_storage_area > min_storage_area_threshold){
            pthread_cond_wait(&in_storage_area_cond, &in_storage_area_mutex);
        }
        printf("\033[1;32m"); // set the color to green
        printf("#################### Storage Area is Less than minimum now ####################\n");
        printf("\033[0m");// reset the color to the default
        //in_box = 0; 
        //if the storage area is below the minimum threshold again, get all lines back to work
        for(int line=0; line<NUMBER_OF_LINES; line++) { 
            pthread_mutex_lock(&suspend_flag_mutex[line]);
            suspend_flag[line] = 0; 
            pthread_mutex_unlock(&suspend_flag_mutex[line]);
        }
        pthread_mutex_unlock(&in_storage_area_mutex);
    }
}
//this function is used by the trucks, to transfer boxes from the storage area to the destination, and be busy for a cetain amount of time
void truck_function(int *arg) {
    //passing the truck number for each thread
    int truck_num = (int) arg; 
    while(1){
        //getting this specific turck mutex, to check its current capacity
        pthread_mutex_lock(&truck_mutex[truck_num]);
        //if max truck capacity is reached, this truck need to move
        if( truck_current_capacity[truck_num] >= truck_capacity){ 
            printf("Truck NO.%d | Full, Ready to go.\n",truck_num);
            //the amount of the transfer time
            sleep(truck_processing_time);
            truck_current_capacity[truck_num] = 0;
            printf("Truck NO.%d | Back to track.\n",truck_num);
        }
        //realeaing the mutex
        pthread_mutex_unlock(&truck_mutex[truck_num]);
    }
}

//this function is used by the HR, to keep checking the gains and expenses and to infrom the CEO accordingly
void HR_function(){
    //Need to keep calculating the expenses and gains. 
    //Expensed are calculated using sum of salaries for the defined number of employees and the expenses by each laptop
    //Gains are calculated using the gain of each laptop
    int expenses = 0;
    int gains = 0;
    while(1){
        sleep(15);
        pthread_mutex_lock(&CEO_HR_mutex);
        //pthread_mutex_lock(&simulation_mutex);
        expenses = salary_CEO + salary_HR + (salary_loading_employee * NUMBER_OF_LOADING_EMPLOYESS) + (salary_storage_area_keeper * NUMBER_OF_STORAGE_AREA_KEEPERS) + (salary_driver_employee * NUMBER_OF_TRUCKS) + (salary_storage_employee * (total_number_of_storage_employees - CEO_HR)) + (salary_technical_employee * (total_number_of_technical_employees-(CEO_HR*NUMBER_OF_TECHNICAL_EMPLOYEES_PER_LINE)));
        for(int line=0; line<NUMBER_OF_LINES;line++){
            pthread_mutex_lock(&in_box_mutex[line]); //to have a precise calculation, equalling the current produced laptops with no extras
            expenses += (ready_laptops_counter[line]*laptop_cost);
            gains += (ready_laptops_counter[line]*laptop_price);
            pthread_mutex_unlock(&in_box_mutex[line]);
        }
        printf("\033[1;34m"); // set the color to cyan
        printf("HR | Expenses: %d\t Gains: %d \n",expenses,gains);
        printf("\033[0m");// reset the color to the default
        //sleep(5);
        //if gain is max than the user_defined max gain, simulation should end, so signal simulation_cond is sent
        if(gains > max_gains_threshold){
            pthread_mutex_lock(&simulation_mutex);
            pthread_cond_signal(&simulation_cond);
            pthread_mutex_unlock(&simulation_mutex);
        }
        //if profit is less than the user_defined min_threshold, infrom the CEO, using signal
        else if(gains - expenses < min_profit_threshold){
            printf("\033[1;31m"); // set the color to red 
            printf("HR | Need to suspend one line\n");
            printf("\033[0m");// reset the color to the default
            CEO_HR ++;
            pthread_cond_signal(&CEO_HR_cond);
        }
        //if profit is greater than the user_defined accepted_profit, infrom the CEO 
        else if(gains - expenses > accepted_profit_threshold){
            printf("\033[1;32m"); // set the color to green
            printf("HR | Can get a line to work again.\n");
            printf("\033[0m");// reset the color to the default
            CEO_HR = 0;
            pthread_cond_signal(&CEO_HR_cond);
        }
        pthread_mutex_unlock(&CEO_HR_mutex);
        //pthread_mutex_unlock(&simulation_mutex);
    }
}
//this function is used by the CEO to suspend and expand the working lines as follows: 
void CEO_function(){
    while(1){
        pthread_mutex_lock(&CEO_HR_mutex);
        pthread_cond_wait(&CEO_HR_cond, &CEO_HR_mutex);
        //if after receiving a signal from the HR, and the varialble CEO_HR is zero, then get all lines back to work 
        if(CEO_HR == 0){
            printf("\033[1;32m"); // set the color to green
            printf("CEO | Will get all lines to work again.\n");
            printf("\033[0m");// reset the color to the default
            for(int line=0; line<NUMBER_OF_LINES; line++) {
                pthread_mutex_lock(&suspend_flag_mutex[line]);
                suspend_flag[line] = 0; 
                pthread_mutex_unlock(&suspend_flag_mutex[line]);
            }    
        }
        else {
            //if  after receiving a signal from the HR, and the varialble CEO_HR is non-zero, then suspend lines with number equal to that in CEO_HR variable
            for(int line=0; line<NUMBER_OF_LINES; line++) { // suspend all lines 
                pthread_mutex_lock(&suspend_flag_mutex[line]);
                if(suspend_flag[line] == 0){
                    suspend_flag[line] = 1; 
                    printf("\033[1;31m"); // set the color to red 
                    pthread_mutex_unlock(&suspend_flag_mutex[line]);
                    printf("CEO  | Will suspend line NO.%d\n", line);
                    printf("\033[0m");// reset the color to the default
                    break;
                }
                pthread_mutex_unlock(&suspend_flag_mutex[line]);
            }
        }
        //if the suspended line are more than 50% of the available lines then end the simulation
        pthread_mutex_lock(&simulation_mutex); 
        if(CEO_HR > NUMBER_OF_LINES/2){
            pthread_cond_signal(&simulation_cond);
        }
        pthread_mutex_unlock(&simulation_mutex);
        pthread_mutex_unlock(&CEO_HR_mutex);
    }
}
