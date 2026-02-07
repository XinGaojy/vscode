// stack_overflow_demo.cpp
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <execinfo.h>
#include <signal.h>
#include <ucontext.h>
#include <unistd.h>

// 设置栈大小
#include <sys/resource.h>
void set_stack_size() {
    rlimit limit;
    getrlimit(RLIMIT_STACK, &limit);
    std::cout << "当前栈限制: " << limit.rlim_cur << " 字节" << std::endl;
    
    // 尝试设置小栈以便更容易触发溢出
    limit.rlim_cur = 1024 * 1024;  // 1MB
    setrlimit(RLIMIT_STACK, &limit);
    
    getrlimit(RLIMIT_STACK, &limit);
    std::cout << "新栈限制: " << limit.rlim_cur << " 字节" << std::endl;
}

// 信号处理器
void signal_handler(int sig, siginfo_t* info, void* context) {
    std::cerr << "\n=== 捕获信号 " << sig << " ===" << std::endl;
    
    if (sig == SIGSEGV) {
        std::cerr << "段错误 (SIGSEGV)" << std::endl;
        std::cerr << "地址: " << info->si_addr << std::endl;
        
        // 检查是否是栈溢出
        ucontext_t* uc = (ucontext_t*)context;
        void* sp = (void*)uc->uc_mcontext.gregs[REG_RSP];
        std::cerr << "栈指针: " << sp << std::endl;
    } else if (sig == SIGBUS) {
        std::cerr << "总线错误 (SIGBUS)" << std::endl;
    } else if (sig == SIGILL) {
        std::cerr << "非法指令 (SIGILL)" << std::endl;
    } else if (sig == SIGFPE) {
        std::cerr << "算术异常 (SIGFPE)" << std::endl;
    }
    
    // 打印调用栈
    std::cerr << "\n调用栈:" << std::endl;
    void* callstack[50];
    int frames = backtrace(callstack, 50);
    char** symbols = backtrace_symbols(callstack, frames);
    
    for (int i = 0; i < frames; ++i) {
        std::cerr << "  #" << i << " " << symbols[i] << std::endl;
    }
    free(symbols);
    
    exit(1);
}

void setup_signal_handler() {
    struct sigaction sa;
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&sa.sa_mask);
    
    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGBUS, &sa, nullptr);
    sigaction(SIGILL, &sa, nullptr);
    sigaction(SIGFPE, &sa, nullptr);
}

// 1. 无限递归导致栈溢出
void infinite_recursion(int depth) {
    char buffer[1024];  // 每个调用分配1KB栈空间
    std::cout << "递归深度: " << depth << "，栈地址: " << (void*)&buffer << std::endl;
    
    if (depth % 100 == 0) {
        std::cout << "深度: " << depth << std::endl;
    }
    
    // 无限递归
    infinite_recursion(depth + 1);
}

// 2. 大数组导致栈溢出
void large_stack_allocation() {
    std::cout << "\n分配大数组..." << std::endl;
    
    // 在栈上分配一个大数组
    char huge_buffer[8 * 1024 * 1024];  // 8MB，可能超过栈限制
    
    std::cout << "数组地址: " << (void*)huge_buffer << std::endl;
    std::cout << "数组大小: " << sizeof(huge_buffer) << " 字节" << std::endl;
    
    // 写入数据（可能触发段错误）
    for (size_t i = 0; i < sizeof(huge_buffer); i += 4096) {
        huge_buffer[i] = 'A';
    }
    
    std::cout << "大数组分配成功" << std::endl;
}

// 3. 递归数据结构导致栈溢出
struct TreeNode {
    int value;
    TreeNode* left;
    TreeNode* right;
    
    TreeNode(int v) : value(v), left(nullptr), right(nullptr) {}
    
    // 深度递归遍历
    int depth() {
        int left_depth = left ? left->depth() + 1 : 0;
        int right_depth = right ? right->depth() + 1 : 0;
        return std::max(left_depth, right_depth);
    }
    
    // 可能导致栈溢出的深度递归
    int deep_recursion() {
        if (!left && !right) return 1;
        int left_val = left ? left->deep_recursion() : 0;
        int right_val = right ? right->deep_recursion() : 0;
        return left_val + right_val + 1;
    }
};

// 创建深树
TreeNode* create_deep_tree(int depth) {
    if (depth <= 0) return nullptr;
    
    TreeNode* node = new TreeNode(depth);
    node->left = create_deep_tree(depth - 1);
    node->right = create_deep_tree(depth - 1);
    return node;
}

void test_deep_tree() {
    std::cout << "\n测试深度树..." << std::endl;
    
    // 创建深度为1000的树
    TreeNode* root = create_deep_tree(1000);
    
    try {
        // 深度优先遍历可能导致栈溢出
        std::cout << "计算深度..." << std::endl;
        int d = root->depth();
        std::cout << "树深度: " << d << std::endl;
        
        std::cout << "深度递归计算..." << std::endl;
        int val = root->deep_recursion();
        std::cout << "递归值: " << val << std::endl;
    } catch (...) {
        std::cout << "捕获异常" << std::endl;
    }
    
    // 注意：这里应该删除树，但为了演示省略
}

// 4. 尾递归优化测试
void tail_recursion(int n, int sum = 0) {
    if (n <= 0) {
        std::cout << "尾递归结果: " << sum << std::endl;
        return;
    }
    
    // 尾递归调用
    tail_recursion(n - 1, sum + n);
}

// 5. 使用迭代避免栈溢出
void iterative_solution(int n) {
    std::cout << "\n使用迭代计算 1 到 " << n << " 的和" << std::endl;
    
    int sum = 0;
    for (int i = 1; i <= n; ++i) {
        sum += i;
    }
    
    std::cout << "迭代结果: " << sum << std::endl;
}

// 6. 使用显式栈
#include <stack>
void dfs_with_explicit_stack(TreeNode* root) {
    if (!root) return;
    
    std::stack<TreeNode*> node_stack;
    node_stack.push(root);
    
    while (!node_stack.empty()) {
        TreeNode* node = node_stack.top();
        node_stack.pop();
        
        std::cout << "访问节点: " << node->value << std::endl;
        
        if (node->right) node_stack.push(node->right);
        if (node->left) node_stack.push(node->left);
    }
}

int main(int argc, char* argv[]) {
    setup_signal_handler();
    set_stack_size();
    
    std::cout << "=== 栈溢出演示程序 ===" << std::endl;
    std::cout << "参数: " << (argc > 1 ? argv[1] : "默认") << std::endl;
    
    if (argc > 1) {
        std::string test = argv[1];
        
        if (test == "recursion") {
            std::cout << "\n测试无限递归..." << std::endl;
            infinite_recursion(0);
        } 
        else if (test == "large_array") {
            std::cout << "\n测试大数组..." << std::endl;
            large_stack_allocation();
        } 
        else if (test == "deep_tree") {
            std::cout << "\n测试深度树..." << std::endl;
            test_deep_tree();
        } 
        else if (test == "tail") {
            std::cout << "\n测试尾递归..." << std::endl;
            tail_recursion(10000);
        } 
        else if (test == "iterative") {
            std::cout << "\n测试迭代..." << std::endl;
            iterative_solution(10000);
        }
    } else {
        std::cout << "使用方法:" << std::endl;
        std::cout << "  ./stack_overflow_demo recursion    # 测试无限递归" << std::endl;
        std::cout << "  ./stack_overflow_demo large_array  # 测试大数组" << std::endl;
        std::cout << "  ./stack_overflow_demo deep_tree     # 测试深度树" << std::endl;
        std::cout << "  ./stack_overflow_demo tail         # 测试尾递归" << std::endl;
        std::cout << "  ./stack_overflow_demo iterative    # 测试迭代" << std::endl;
    }
    
    return 0;
}
