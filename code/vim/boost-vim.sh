# 保存为 boost-vim.sh && chmod +x boost-vim.sh && ./boost-vim.sh
#!/bin/bash
set -e
# 1. 装 vim-plug
curl -fLo ~/.vim/autoload/plug.vim --create-dirs \
  https://raw.githubusercontent.com/junegunn/vim-plug/master/plug.vim
# 2. 生成最强 .vimrc
cat > ~/.vimrc <<'EOF'
"================== 基础 VSCode 化 ==================
set nocompatible
filetype plugin indent on
syntax on
set number relativenumber
set cursorline
set mouse=a                 " 全鼠标支持：点击、滚轮、选区 [^42^]
set clipboard=unnamedplus   " 系统剪贴板
set expandtab shiftwidth=2 softtabstop=2
set autoindent smartindent
set wrap linebreak          " 长行自动换行且不断单词
set showbreak=↪\ 
set incsearch hlsearch
set ignorecase smartcase
set cmdheight=2
set signcolumn=yes          " 给 Git 留 gutter
set termguicolors           " 真彩色终端
colorscheme desert

"================== 插件管理 ==================
call plug#begin('~/.vim/plugged')

" 文件树（VSCode 侧边栏）
Plug 'preservim/nerdtree'
" 模糊查找（Ctrl+P 效果）
Plug 'junegunn/fzf', { 'do': { -> fzf#install() } }
Plug 'junegunn/fzf.vim'
" 状态栏
Plug 'vim-airline/vim-airline'
" 语法高亮增强
Plug 'sheerun/vim-polyglot'
" Git 状态条
Plug 'airblade/vim-gitgutter'
" 多光标（VSCode Alt+Click）
Plug 'mg979/vim-visual-multi'
" 自动补全（LSP 级）
Plug 'neoclide/coc.nvim', {'branch': 'release'}
" 注释行（VSCode Ctrl+/）
Plug 'tpope/vim-commentary'
" 成对编辑
Plug 'tpope/vim-surround'
" 格式化
Plug 'sbdchd/neoformat'

call plug#end()

"================== 键位映射（VSCode 手感） ==================
let mapleader=" "
" 文件树
nnoremap <C-n> :NERDTreeToggle<CR>
" 模糊找文件
nnoremap <C-p> :Files<CR>
" 全局搜索
nnoremap <leader>f :Rg<CR>
" 多光标
nnoremap <C-LeftMouse> <Plug>(VM-Mouse-Cursor)
nnoremap <C-RightMouse> <Plug>(VM-Mouse-Word)
" 保存/退出
nnoremap <C-s> :w<CR>
inoremap <C-s> <Esc>:w<CR>
" 换行不断缩进（o/O 自动续行）
nnoremap o o<Esc>^Da
nnoremap O O<Esc>^Da

"================== coc.nvim 配置 ==================
inoremap <expr> <Tab> pumvisible() ? "\<C-n>" : "\<Tab>"
inoremap <expr> <S-Tab> pumvisible() ? "\<C-p>" : "\<S-Tab>"
nnoremap <silent> gd <Plug>(coc-definition)
nnoremap <silent> gy <Plug>(coc-type-definition)
nnoremap <silent> gr <Plug>(coc-references)
nnoremap <silent> K :call CocActionAsync('doHover')<CR>

"================== 鼠标增强 ==================
" 双击选中单词
nnoremap <2-LeftMouse> viw
" 右键菜单（NERDTree）
nnoremap <RightMouse> :call nerd#contextMenu()<CR>

"================== 长行换行可视化 ==================
set wrap
set linebreak
set showbreak=↪\
set breakindent
set breakindentopt=shift:2
EOF

# 3. 装插件
vim -c 'PlugInstall | q | q'
echo "✅ 最强终端 Vim 已就绪！打开 vim 体验 VSCode 级操作"
