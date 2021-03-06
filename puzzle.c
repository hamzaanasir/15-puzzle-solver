#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <math.h>
#include <assert.h>

/**
 * DESCRIPTION
 *
 * node data structure: containing state, g, f
 * you can extend it with more information if needed
 */
typedef struct node{
	int state[16];
	int g;
	int f;
} node;

/**
 * Global Variables
 */

// used to track the position of the blank in a state,
// so it doesn't have to be searched every time we check if an operator is applicable
// When we apply an operator, blank_pos is updated
int blank_pos;

// Initial node of the problem
node initial_node;

// Statistics about the number of generated and expendad nodes
unsigned long generated;
unsigned long expanded;

// Maximum number
#define MAXIMUM INT_MAX

// Maximum moves
#define MAX_OPERATIONS 4

// State size
#define PUZZLE_SIZE 16

// Invalid moves
#define NOT_VALID_OP -1
/**
 * The id of the four available actions for moving the blank (empty slot). e.x.
 * Left: moves the blank to the left, etc.
 */

#define LEFT 0
#define RIGHT 1
#define UP 2
#define DOWN 3

/*
 * Helper arrays for the applicable function
 * applicability of operators: 0 = left, 1 = right, 2 = up, 3 = down
 */
int ap_opLeft[]  = { 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1 };
int ap_opRight[]  = { 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0 };
int ap_opUp[]  = { 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
int ap_opDown[]  = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 };
int *ap_ops[] = { ap_opLeft, ap_opRight, ap_opUp, ap_opDown };

/* print state */
void print_state( int* s )
{
	int i;

	for( i = 0; i < 16; i++ )
		printf( "%2d%c", s[i], ((i+1) % 4 == 0 ? '\n' : ' ') );
}

void printf_comma (long unsigned int n) {
    if (n < 0) {
        printf ("-");
        printf_comma (-n);
        return;
    }
    if (n < 1000) {
        printf ("%lu", n);
        return;
    }
    printf_comma (n/1000);
    printf (",%03lu", n%1000);
}

/* return the manhattan distance from a point in state to a point in goal */
int ManhattanDistance(int current_x, int final_x, int current_y, int final_y) {
	return ((abs(current_x - final_x)) + (abs(current_y - final_y)));
}

/* return the sum of manhattan distances from state to goal */
int manhattan( int* state )
{
	int initial_x=0, final_x=0, initial_y=0, final_y=0;
	int i;
	int sum = 0;
	// floor function takes a double and returns the largest integer
	// less than or equal to the argument supplied
	for (i=0; i<PUZZLE_SIZE; i++) {
		if (state[i] == 0) {
			// do nothing
			sum += 0;
		}
		else {
			initial_x = i%4;
			initial_y = floor(i/4);
			final_x = state[i]%4;
			final_y = floor(state[i]/4);
			sum += ManhattanDistance(initial_x, final_x, initial_y, final_y);
		}
  }
	return sum;
}

/* return 1 if op is applicable in state, otherwise return 0 */
int applicable( int op )
{
       	return( ap_ops[op][blank_pos] );
}

/* apply operator */
void apply( node* n, int op )
{
	int t;

	//find tile that has to be moved given the op and blank_pos
	t = blank_pos + (op == 0 ? -1 : (op == 1 ? 1 : (op == 2 ? -4 : 4)));

	//apply op
	n->state[blank_pos] = n->state[t];
	n->state[t] = 0;

	//update blank pos
	blank_pos = t;
}

/* return the minimum of the two inputs */
int select_minimum(int f, int B) {
	if (f <= B) {
		return f;
	}
	else if (f > B) {
		return B;
	}
	return 0;
}

/* Recursive IDA */
node* ida( node* node, int threshold, int* newThreshold, int pre ) {

	int i;
	int current_position = blank_pos;
	struct node* newnode = (struct node*) malloc(sizeof(struct node));
	assert(newnode);

	for(i = 0; i < MAX_OPERATIONS; i++) {
		// discarding moves that make the current state go back
		// to the previous state
		if ((pre != NOT_VALID_OP) && ((i + pre == 1) || (i + pre == 5))) {
			continue;
		}
		if(applicable(i)) {

			struct node* r = NULL;

			// copy the state of node into the state of newnode
			memcpy(newnode->state, node->state, sizeof(int)*PUZZLE_SIZE);

			apply(newnode, i);
			generated++;
			newnode->g = node->g + 1;
			newnode->f = newnode->g + manhattan(newnode->state);

			// setting the new threshold
			if(newnode->f > threshold) {
				*(newThreshold) = select_minimum(*(newThreshold), newnode->f);
			}
			else {
				if(manhattan(newnode->state) == 0) {
					return newnode;
				}
				expanded++;
				// recursive call to the function
				r = ida(newnode, threshold, newThreshold, i);
				if(r!=NULL) {
					return r;
				}
			}
			// update the blank position
			blank_pos = current_position;
		}
	}
	// free the node
	free(newnode);
	return( NULL );
}


/* main IDA control loop */
int IDA_control_loop(  ){
	node* r = NULL;
	int threshold;
	int newthreshold;

	/* initialize statistics */
	generated = 0;
	expanded = 0;

	/* compute initial threshold B */
	initial_node.f = threshold = manhattan( initial_node.state );
	printf( "Initial Estimate = %d\nThreshold = ", threshold );

	while(r == NULL){
		node n;

		// setting the threshold to the maximum integer
		newthreshold = MAXIMUM;
		// copy the state of initial node to the state of node
		memcpy(n.state, initial_node.state, sizeof(int)*PUZZLE_SIZE);

		n.g = 0;

		// recursive call to the function
		// not passing a valid move to the initial call so that the code
		// does not miss optimal solution
		r = ida(&n, threshold, &newthreshold, NOT_VALID_OP);

		if(r == NULL) {
			// update the threshold
			threshold = newthreshold;
			printf(" %d ", threshold);
		}
	}
	if(r) {
		int temp = r->g;
		// free the node
		free(r);
		return temp;
	}
	else {
		return -1;
	}
}

static inline float compute_current_time()
{
	struct rusage r_usage;

	getrusage( RUSAGE_SELF, &r_usage );
	float diff_time = (float) r_usage.ru_utime.tv_sec;
	diff_time += (float) r_usage.ru_stime.tv_sec;
	diff_time += (float) r_usage.ru_utime.tv_usec / (float)1000000;
	diff_time += (float) r_usage.ru_stime.tv_usec / (float)1000000;
	return diff_time;
}

int main( int argc, char **argv )
{
	int i, solution_length;

	/* check we have a initial state as parameter */
	if( argc != 2 )
	{
		fprintf( stderr, "usage: %s \"<initial-state-file>\"\n", argv[0] );
		return( -1 );
	}


	/* read initial state */
	FILE* initFile = fopen( argv[1], "r" );
	char buffer[256];

	if( fgets(buffer, sizeof(buffer), initFile) != NULL ){
		char* tile = strtok( buffer, " " );
		for( i = 0; tile != NULL; ++i )
			{
				initial_node.state[i] = atoi( tile );
				blank_pos = (initial_node.state[i] == 0 ? i : blank_pos);
				tile = strtok( NULL, " " );
			}
	}
	else{
		fprintf( stderr, "Filename empty\"\n" );
		return( -2 );

	}

	if( i != 16 )
	{
		fprintf( stderr, "invalid initial state\n" );
		return( -1 );
	}

	/* initialize the initial node */
	initial_node.g=0;
	initial_node.f=0;

	print_state( initial_node.state );


	/* solve */
	float t0 = compute_current_time();

	solution_length = IDA_control_loop();

	float tf = compute_current_time();

	/* report results */
	printf( "\nSolution = %d\n", solution_length);
	printf( "Generated = ");
	printf_comma(generated);
	printf("\nExpanded = ");
	printf_comma(expanded);
	printf( "\nTime (seconds) = %.2f\nExpanded/Second = ", tf-t0 );
	printf_comma((unsigned long int) expanded/(tf+0.00000001-t0));
	printf("\n\n");

	/* aggregate all executions in a file named report.dat, for marking purposes */
	FILE* report = fopen( "report.dat", "a" );

	fprintf( report, "%s", argv[1] );
	fprintf( report, "\n\tSoulution = %d, Generated = %lu, Expanded = %lu", solution_length, generated, expanded);
	fprintf( report, ", Time = %f, Expanded/Second = %f\n\n", tf-t0, (float)expanded/(tf-t0));
	fclose(report);

	return( 0 );
}
