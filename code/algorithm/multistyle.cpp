//


#include <stdio.h>
#include <stdlib.h>

// 1. 定义虚函数表结构
typedef struct {
    void (*speak)(void*);
    void (*eat)(void*);
} VTable;

// 2. 基类结构
typedef struct {
    VTable* vptr;  // 虚函数表指针
    char name[20];
} Animal;

// 3. 派生类：Dog
typedef struct {
    Animal base;    // 基类对象（继承）
    int bone_count;
} Dog;

// 4. 派生类：Cat
typedef struct {
    Animal base;    // 基类对象（继承）
    int fish_count;
} Cat;

// 5. Dog的虚函数实现
void dog_speak(void* animal) {
    Dog* dog = (Dog*)animal;
    printf("%s says: Woof! Woof! (我有%d根骨头)\n", dog->base.name, dog->bone_count);
}

void dog_eat(void* animal) {
    Dog* dog = (Dog*)animal;
    dog->bone_count++;
    printf("%s 吃了一根骨头，现在有%d根骨头\n", dog->base.name, dog->bone_count);
}

// 6. Cat的虚函数实现
void cat_speak(void* animal) {
    Cat* cat = (Cat*)animal;
    printf("%s says: Meow! Meow! (我有%d条鱼)\n", cat->base.name, cat->fish_count);
}

void cat_eat(void* animal) {
    Cat* cat = (Cat*)animal;
    cat->fish_count++;
    printf("%s 吃了一条鱼，现在有%d条鱼\n", cat->base.name, cat->fish_count);
}

// 7. 虚函数表定义
VTable dog_vtable = { dog_speak, dog_eat };
VTable cat_vtable = { cat_speak, cat_eat };

// 8. 构造函数
Dog* create_dog(const char* name) {
    Dog* dog = (Dog*)malloc(sizeof(Dog));
    dog->base.vptr = &dog_vtable;
    snprintf(dog->base.name, sizeof(dog->base.name), "%s", name);
    dog->bone_count = 0;
    return dog;
}

Cat* create_cat(const char* name) {
    Cat* cat = (Cat*)malloc(sizeof(Cat));
    cat->base.vptr = &cat_vtable;
    snprintf(cat->base.name, sizeof(cat->base.name), "%s", name);
    cat->fish_count = 0;
    return cat;
}

// 9. 多态接口函数
void animal_speak(Animal* animal) {
    animal->vptr->speak(animal);
}

void animal_eat(Animal* animal) {
    animal->vptr->eat(animal);
}
 :i
// 10. 析构函数
void destroy_animal(Animal* animal) {
    free(animal);
}

int main() {
    printf("=== C语言模拟多态演示 ===\n\n");
    
    // 创建不同动物对象，但用基类指针引用
    Animal* animals[3];
    
    animals[0] = (Animal*)create_dog("旺财");
    animals[1] = (Animal*)create_cat("咪咪");
    animals[2] = (Animal*)create_dog("小黑");
    
    // 多态调用 - 相同的接口，不同的行为
    for (int i = 0; i < 3; i++) {
        animal_speak(animals[i]);
        animal_eat(animals[i]);
        animal_eat(animals[i]);
        printf("\n");
    }
    
    // 释放内存
    for (int i = 0; i < 3; i++) {
        destroy_animal(animals[i]);
    }
    
    return 0;
}






struct Vtable{
    void (*speak)(void *);
    void (*eat)(void *);
};

typedef struct{
    VTable* vptr;
    char *name[20];

}Animal;

typedef struct {
    Animal base;
    int bone_count;
}Dog;

typedef struct {
    Animal base;
    int fish_count;
}Cat;

void dog_speak(void *animal){
    Dog* dog=(Dog*)animal;
}

void dog_eat(void *animal){
    Dog* dog=(Dog*)animal;
    dog->bone_count++;
}

void cat_speak(void *animal){
    Cat* cat=(Cat*)animal;
    cat->fish_count++;
}

void cat_eat(void *animal){
    Cat *cat=(Cat*)animal;
    cat->fish_count++;
}
Vtable dog_table={dog_speak,dog_eat};
VTable cat_table={cat_speak,cat_eat};

Dog * create_dog(const char *name){
    Dog* dog=(Dog*)malloc(sizeof(Dog));
    dog->base.vptr=&dog_vtable;
    dog->bone_count=0;
    return dog;
}

cat *create_cat(const char *name){
    Cat* cat=(Cat*)malloc(sizeof(Cat));
    cat->base.vptr=&cat_vtable;
    cat->fish_count=0;
    reurn cat;
}

void animal_speak(Animal*animal){
    animal->vptr->speak(animal);
}


void animal_eat(Animal* animal){
    animal->vptr->speak(animal);
}


void destroy_animal(Animal* animal){
    free(animal);
}







