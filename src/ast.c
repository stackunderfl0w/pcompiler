#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Transforms/Scalar.h>

#include "ast.h"

#include "scope.h"


ast_node* create_ast_node(int node_type, ...){
    ast_node* node=calloc(sizeof(ast_node),1);
    node->type=node_type;
    va_list args;
    va_start(args, node_type);
    switch(node_type){
    case AST_IF: case AST_CAST: case AST_GREATER_THAN ... AST_MOD: case AST_ASSIGNMENT: case AST_ELIF: case AST_WHILE:case AST_ARRAY_ACCESS:case AST_FUNCTION_CALL:
            node->L=va_arg(args, ast_node*);
            node->R=va_arg(args, ast_node*);
            break;
        case AST_INTEGER:
            //node->ival=atol(va_arg(args, char*));
            sscanf(va_arg(args, char*),"%zx",&node->ival);
            break;
        case AST_FLOAT:
            node->fval=atof(va_arg(args, char*));
            break;
        case AST_DEFINITION:
            node->L=(ast_node *)strdup(va_arg(args, char*));
            node->name=strdup(va_arg(args, char*));
            break;
        case AST_IDENTIFIER:
            node->name=strdup(va_arg(args, char*));
            break;
        case AST_FUNCTION:{
            ast_node* def=va_arg(args, ast_node*);
            node->name=strdup(def->name);
            node->vtype=strdup((char*)def->L);
            node->L=va_arg(args, ast_node*);
            node->R=va_arg(args, ast_node*);
            break;
        }
        case AST_BREAK:break;
        case AST_RETURN: case AST_DEREFERENCE: case AST_POST_INC ... AST_PRE_DEC:
            node->L=va_arg(args,ast_node*);
            break;
        default:
            printf("create_ast_node: UNMATCHED node type%d\n",node_type);
            exit(0);
    }
    va_end(args);
    return node;
}

LLVMTypeRef str_to_type(char* type){
    if(!strcmp(type,"i32")){
        return LLVMInt32Type();
    } else if(!strcmp(type,"f32")){
        return LLVMFloatType();
    } else{
        fprintf(stderr,"Error:undefined type (%s)\n",type);
        exit(0);
    }
}

LLVMValueRef build_alloca(char* name, LLVMBuilderRef builder, LLVMTypeRef type) {
    return LLVMBuildAlloca(builder, type, name);
}

LLVMValueRef build_array_alloca(char* name, LLVMBuilderRef builder, LLVMTypeRef type, int size){
    LLVMTypeRef arrayType = LLVMArrayType(type, size);
    return LLVMBuildAlloca(builder, arrayType, name);
}
LLVMModuleRef module;

LLVMValueRef ast_node_llvm(struct ast_node* node, LLVMBuilderRef builder, scope* symbols);
LLVMValueRef ast_node_llvm_statements(ast_node* node, LLVMBuilderRef builder, scope* symbols);

LLVMValueRef ast_node_llvm_function(ast_node* node, LLVMBuilderRef builder, scope* symbols){
    //check for existing function
    if(scope_get_top(symbols,node->name)){
        printf("ERROR: fn declaration: function %s already defined in current scope\n", node->name);
        exit(1);
    }

    //get function args
    int arg_count=0;
    for (ast_node* args=node->L; args ; args=args->NEXT, arg_count++);

    LLVMTypeRef param_types[arg_count];
    for (int i = 0; i < arg_count; ++i)
        param_types[i]=LLVMInt32Type();
    int i=0;
    for (ast_node* args=node->L; args ; args=args->NEXT, i++)
        param_types[i++]= str_to_type((char*)args->L);

    //create function
    LLVMTypeRef fn_type = LLVMFunctionType(str_to_type(node->vtype), param_types, arg_count, 0);
    LLVMValueRef foo_fn = LLVMAddFunction(module, node->name, fn_type);

    scope_insert(symbols,node->name, foo_fn);

    char fn_name[256]={0};
    sprintf(fn_name,"%s_entry",node->name);
    LLVMBasicBlockRef entry_blk = LLVMAppendBasicBlock(foo_fn, fn_name);

    LLVMPositionBuilderAtEnd(builder, entry_blk);
    symbols= scope_create(symbols);
    int index=0;
    for (ast_node* args=node->L; args ; args=args->NEXT, index++) {
        LLVMValueRef arg = LLVMGetParam(foo_fn, index);
        LLVMValueRef var= build_alloca(args->name, builder, LLVMTypeOf(arg));
        LLVMBuildStore(builder,arg,var);
        scope_insert(symbols,args->name, var);
    }

    ast_node_llvm_statements(node->R, builder, symbols);
    scope_pop(&symbols);
    return foo_fn;
}
LLVMValueRef ast_node_llvm_function_call(ast_node* node, LLVMBuilderRef builder, scope* symbols){
    LLVMValueRef fn= scope_get(symbols, node->L->name);
    //LLVMValueRef fn_ref= ast_node_llvm(node->L, builder, symbols);
    int arg_count=0;
    for (ast_node* args=node->R; args ; args=args->NEXT, arg_count++);

    LLVMValueRef call_args[1];
    int index=0;
    for (ast_node* args=node->R; args ; args=args->NEXT, index++){
        call_args[index]= ast_node_llvm(args, builder, symbols);
    }
    //fuck the documentation. fn_type needs to be the function type not the return type for some fucking reason.
    //LLVMGetCalledFunctionType exists and does not work
    LLVMValueRef result = LLVMBuildCall2(builder,LLVMGetElementType((LLVMTypeRef)fn),fn,call_args,arg_count,"call_result");
    return result;
}

LLVMValueRef ast_node_variable_access(ast_node* node, LLVMBuilderRef builder, scope* symbols){
    LLVMValueRef var=scope_get(symbols, node->name);
    if (!var) {
        fprintf(stderr, "Unknown variable: %s\n", node->name);
        return LLVMGetUndef(LLVMInt32Type());
    }
    return var;
}


LLVMValueRef ast_node_array_access(ast_node* node, LLVMBuilderRef builder, scope* symbols) {
    LLVMValueRef var=scope_get(symbols, node->L->name);
    if (!var) {
        fprintf(stderr, "Unknown variable: %s\n", node->name);
        return LLVMGetUndef(LLVMInt32Type());
    }
    LLVMValueRef index= ast_node_llvm(node->R,builder,symbols);
    LLVMValueRef indices[] = { index }; // Array indices
    unsigned numIndices = sizeof(indices) / sizeof(indices[0]);

    return LLVMBuildGEP2(builder, LLVMInt32Type(), var, indices, numIndices, "gep_result");
}

LLVMValueRef ast_node_llvm_dereference(ast_node* node, LLVMBuilderRef builder, scope* symbols) {
    LLVMValueRef var=ast_node_llvm(node->L, builder, symbols);
    return LLVMBuildLoad2(builder, LLVMGetAllocatedType(var), var, "deref");
}
LLVMValueRef ast_node_llvm_cast(ast_node* node, LLVMBuilderRef builder, scope* symbols) {
    LLVMValueRef var=ast_node_llvm(node->R, builder, symbols);
    LLVMTypeRef srcTy= LLVMTypeOf(var);
    LLVMTypeRef destTy=str_to_type((char*)node->L);
    //no conversion needed
    if(srcTy==destTy){
        return var;
    }
    LLVMOpcode conv_op=0;

    if(srcTy==LLVMInt32Type()&&destTy==LLVMFloatType()){
        conv_op=LLVMSIToFP;
    }
    else if(srcTy==LLVMFloatType()&&destTy==LLVMInt32Type()){
        conv_op=LLVMFPToSI;
    }

    return LLVMBuildCast(builder,conv_op ,var, destTy, "cast_result");
}


LLVMValueRef ast_node_llvm_definition(ast_node* node, LLVMBuilderRef builder, scope* symbols){
    if(scope_get_top(symbols,node->name)){
        printf("ERROR: Var %s already defined in current scope\n", node->name);
        exit(1);
    }

    LLVMValueRef var;
    if(symbols->previous){
        var=build_alloca(node->name, builder, str_to_type((char*)node->L));
    }
    else{
        var=LLVMAddGlobal(module,str_to_type((char*)node->L),node->name);

        //LLVMSetLinkage(var,LLVMInternalLinkage);
        //LLVMSetVisibility(var,LLVMHiddenVisibility);
        //todo dso_local
    }
    scope_insert(symbols,node->name, var);
    return var;
}

LLVMValueRef ast_node_llvm_array_definition(ast_node* node, LLVMBuilderRef builder, scope* symbols){
    if(scope_get_top(symbols,node->name)){
        printf("ERROR: Var %s already in scope top\n", node->name);
        exit(1);
    }
    LLVMValueRef var= build_array_alloca(node->name, builder, LLVMInt32Type(),node->ival);
    scope_insert(symbols,node->name, var);
    return var;
}


LLVMValueRef ast_node_llvm_assign(ast_node* node, LLVMBuilderRef builder, scope* symbols){
    LLVMValueRef var=ast_node_llvm(node->L,builder,symbols);
    return LLVMBuildStore(builder, ast_node_llvm(node->R,builder, symbols), var);
}

LLVMValueRef ast_node_llvm_return(ast_node* node, LLVMBuilderRef builder, scope* symbols){
    return LLVMBuildRet(builder, ast_node_llvm(node->L,builder, symbols));
}
LLVMValueRef build_i32(int val) {
    return LLVMConstInt(LLVMInt32Type(), val,1);
}
LLVMValueRef build_f32(float val) {
    return LLVMConstReal(LLVMFloatType(), val);
}

LLVMValueRef ast_node_llvm_statements(ast_node* node, LLVMBuilderRef builder, scope* symbols){
    LLVMValueRef ret=NULL;
    do {
        ret=ast_node_llvm(node,builder,symbols);
        //skip unreachable code
        if (node->type==AST_BREAK||node->type==AST_RETURN)
            break;
    }while((node=node->NEXT));
    return ret;
}

LLVMValueRef ast_node_llvm_uniop(ast_node* node, LLVMBuilderRef builder, scope* symbols){
    LLVMValueRef operand = ast_node_llvm(node->L, builder, symbols);
    if (LLVMIsUndef(operand)) {
        return LLVMGetUndef(LLVMInt32Type());
    }
    LLVMValueRef final=NULL;
    switch (node->type){
        case AST_POST_INC...AST_PRE_DEC:{
            //load variable
            LLVMValueRef original = LLVMBuildLoad2(builder, LLVMInt32Type(), operand, "deref");
            //calculate result
            if(node->type==AST_POST_INC||node->type==AST_PRE_INC)
                final = LLVMBuildAdd(builder, original, build_i32(1), "inc_result");
            else if(node->type==AST_POST_DEC||node->type==AST_PRE_DEC)
                final = LLVMBuildSub(builder, original, build_i32(1), "dec_result");
            //store result
            LLVMBuildStore(builder, final, operand);
            //return value to use
            if(node->type==AST_POST_INC||node->type==AST_POST_DEC)
                return original;
            else if(node->type==AST_PRE_INC||node->type==AST_PRE_DEC)
                return final;
        }
        default:
            fprintf(stderr, "llvm_uniop called with incompatible node type (%d)",node->type);
            exit(0);
    }
}

LLVMValueRef ast_node_llvm_binop(ast_node* node, LLVMBuilderRef builder, scope* symbols){
    LLVMValueRef lhs = ast_node_llvm(node->L, builder, symbols);
    LLVMValueRef rhs = ast_node_llvm(node->R, builder, symbols);
    if (LLVMIsUndef(lhs) || LLVMIsUndef(rhs)) {
        return LLVMGetUndef(LLVMInt32Type());
    }
    if(LLVMTypeOf(lhs)!= LLVMTypeOf(rhs)){
        char* lt,*rt;
        lt=LLVMPrintTypeToString(LLVMTypeOf(lhs));
        rt=LLVMPrintTypeToString(LLVMTypeOf(rhs));
        fprintf(stderr,"Error: binop: operands do not have matching types, found (%s) and (%s)\n",lt,rt);
        free(lt);
        free(rt);
        exit(0);
    }

    switch (node->type) {
        case AST_ADD:
            return LLVMBuildAdd(builder, lhs, rhs, "add_result");
        case AST_SUB:
            return LLVMBuildSub(builder, lhs, rhs, "sub_result");
        case AST_MUL:
            if(LLVMGetTypeKind(LLVMTypeOf(lhs))==LLVMIntegerTypeKind)
                return LLVMBuildMul(builder, lhs, rhs, "mul_result");
            else if(LLVMGetTypeKind(LLVMTypeOf(lhs))==LLVMFloatTypeKind)
                return LLVMBuildFMul(builder,lhs,rhs,"fmul_result");
        case AST_DIV:
            if(LLVMGetTypeKind(LLVMTypeOf(lhs))==LLVMIntegerTypeKind)
                return LLVMBuildSDiv(builder, lhs, rhs, "div_result");
            else if(LLVMGetTypeKind(LLVMTypeOf(lhs))==LLVMFloatTypeKind)
                return LLVMBuildFDiv(builder,lhs,rhs,"fdiv_result");
        case AST_MOD:
            return LLVMBuildSRem(builder, lhs, rhs, "mod_result");
        case AST_EQUAL:
            return  LLVMBuildICmp(builder, LLVMIntEQ, lhs, rhs, "eq_result");
            //return LLVMBuildUIToFP(builder, lt_bool, LLVMInt32Type(), "cast_result");
        case AST_NEQUAL:
            return  LLVMBuildICmp(builder, LLVMIntNE, lhs, rhs, "ne_result");
            //return LLVMBuildUIToFP(builder, lt_bool, LLVMInt32Type(), "cast_result");
        case AST_GREATER_THAN:
            return LLVMBuildICmp(builder, LLVMIntSGT, lhs, rhs, "gt_result");
            //return LLVMBuildUIToFP(builder, lt_bool, LLVMInt32Type(), "cast_result");
        case AST_GREATER_THAN_EQ:
            return LLVMBuildICmp(builder, LLVMIntSGE, lhs, rhs, "gte_result");
            //return LLVMBuildUIToFP(builder, lt_bool, LLVMInt32Type(), "cast_result");
        case AST_LESS_THAN:
            return LLVMBuildICmp(builder, LLVMIntSLT, lhs, rhs, "lt_result");
            //return LLVMBuildUIToFP(builder, lt_bool, LLVMInt32Type(), "cast_result");
        case AST_LESS_THAN_EQ:
            return LLVMBuildICmp(builder, LLVMIntSLE, lhs, rhs, "lte_result");
            //return LLVMBuildUIToFP(builder, lt_bool, LLVMInt32Type(), "cast_result");

        default:
            printf("Error: invalid operator: %d\n", node->type);
            exit(1);
            //return LLVMGetUndef(LLVMFloatType());
    }
}
int llvm_block_is_terminated(LLVMBasicBlockRef bb){
    LLVMValueRef terminator = LLVMGetBasicBlockTerminator(bb);
    return (LLVMIsAReturnInst(terminator) || LLVMIsABranchInst(terminator));
}

LLVMValueRef ast_node_llvm_if2(ast_node* node, LLVMBuilderRef builder, scope* symbols) {
    symbols= scope_create(symbols);
    LLVMValueRef cond = ast_node_llvm(node->L, builder, symbols);

    LLVMBasicBlockRef curr_blk = LLVMGetInsertBlock(builder);
    LLVMValueRef curr_fn = LLVMGetBasicBlockParent(curr_blk);
    LLVMBasicBlockRef then_blk = LLVMAppendBasicBlock(curr_fn, "if_then");
    LLVMBasicBlockRef else_blk=NULL;
    if(node->ELSE)
        else_blk = LLVMAppendBasicBlock(curr_fn, "else");

    LLVMBasicBlockRef continue_blk = LLVMAppendBasicBlock(curr_fn, "if_continue");

    LLVMBuildCondBr(builder, cond, then_blk, else_blk?else_blk:continue_blk);


    LLVMPositionBuilderAtEnd(builder, then_blk);


    ast_node_llvm_statements(node->R, builder, symbols);

    if(!llvm_block_is_terminated(then_blk)){
        LLVMBuildBr(builder, continue_blk);
    }

    if(else_blk){
        LLVMPositionBuilderAtEnd(builder, else_blk);
        ast_node_llvm_statements(node->ELSE, builder, symbols);
        if(!llvm_block_is_terminated(else_blk)){
            LLVMBuildBr(builder, continue_blk);
        }
    }

    LLVMPositionBuilderAtEnd(builder, continue_blk);
    scope_pop(&symbols);
    return LLVMBasicBlockAsValue(continue_blk);
}

LLVMValueRef ast_node_llvm_if(ast_node* node, LLVMBuilderRef builder, scope* symbols) {
    scope_push(&symbols);
    LLVMValueRef cond = ast_node_llvm(node->L, builder, symbols);

    LLVMBasicBlockRef curr_blk = LLVMGetInsertBlock(builder);
    LLVMValueRef curr_fn = LLVMGetBasicBlockParent(curr_blk);
    LLVMBasicBlockRef then_blk = LLVMAppendBasicBlock(curr_fn, "if_then");
    LLVMBasicBlockRef continue_blk = LLVMAppendBasicBlock(curr_fn, "if_continue");

    ast_node* n=node;
    while(n->ELIF){
        LLVMBasicBlockRef elif_cond = LLVMInsertBasicBlock(continue_blk,"elif_cond");
        LLVMBuildCondBr(builder, cond, then_blk, elif_cond);

        LLVMPositionBuilderAtEnd(builder, then_blk);
        ast_node_llvm_statements(n->R, builder, symbols);
        if(!llvm_block_is_terminated(then_blk)){
            LLVMBuildBr(builder, continue_blk);
        }

        LLVMPositionBuilderAtEnd(builder, elif_cond);
        cond= ast_node_llvm(n->ELIF->L,builder,symbols);
        then_blk = LLVMInsertBasicBlock(continue_blk,"elif_then");

        n=n->ELIF;
    }
    if(node->ELSE){
        LLVMBasicBlockRef else_blk = LLVMInsertBasicBlock(continue_blk, "else");
        LLVMBuildCondBr(builder, cond, then_blk, else_blk);
        LLVMPositionBuilderAtEnd(builder, else_blk);
        ast_node_llvm_statements(node->ELSE, builder, symbols);
        if(!llvm_block_is_terminated(else_blk)){
            LLVMBuildBr(builder, continue_blk);
        }
    }
    else{
        LLVMBuildCondBr(builder, cond, then_blk, continue_blk);
    }
    LLVMPositionBuilderAtEnd(builder, then_blk);
    ast_node_llvm_statements(n->R, builder, symbols);
    //if(!llvm_block_is_terminated(then_blk)){
    if(!llvm_block_is_terminated(LLVMGetInsertBlock(builder))){
        LLVMBuildBr(builder, continue_blk);
    }

    LLVMPositionBuilderAtEnd(builder, continue_blk);
    scope_pop(&symbols);
    return LLVMBasicBlockAsValue(continue_blk);
}

LLVMBasicBlockRef current_break_to=NULL;

LLVMValueRef ast_node_llvm_while(ast_node* node, LLVMBuilderRef builder, scope* symbols){
    scope_push(&symbols);
    LLVMBasicBlockRef curr_blk = LLVMGetInsertBlock(builder);
    LLVMValueRef curr_fn = LLVMGetBasicBlockParent(curr_blk);

    LLVMBasicBlockRef while_cond_block = LLVMAppendBasicBlock(curr_fn, "while_cond");

    LLVMBuildBr (builder, while_cond_block);

    LLVMPositionBuilderAtEnd(builder, while_cond_block);

    LLVMValueRef cond = ast_node_llvm(node->L, builder, symbols);

    LLVMBasicBlockRef then_blk = LLVMAppendBasicBlock(curr_fn, "while_then");
    LLVMBasicBlockRef continue_blk = LLVMAppendBasicBlock(curr_fn, "while_continue");

    LLVMBasicBlockRef prev_break_to=current_break_to;
    current_break_to=continue_blk;


    LLVMBuildCondBr(builder, cond, then_blk, continue_blk);

    LLVMPositionBuilderAtEnd(builder, then_blk);


    ast_node_llvm_statements(node->R, builder, symbols);

    LLVMBuildBr (builder, while_cond_block);

    LLVMPositionBuilderAtEnd(builder, continue_blk);

    current_break_to=prev_break_to;
    scope_pop(&symbols);
    return NULL;
}

LLVMValueRef ast_node_llvm(ast_node* node, LLVMBuilderRef builder, scope* symbols){
    if(!node)
        return NULL;
    switch (node->type) {
        case AST_FUNCTION:
            return ast_node_llvm_function(node, builder, symbols);
        case AST_FUNCTION_CALL:
            return ast_node_llvm_function_call(node, builder, symbols);
        case AST_RETURN:
            return ast_node_llvm_return(node,builder, symbols);
        case AST_IDENTIFIER:
            return ast_node_variable_access(node, builder, symbols);
        //case AST_FLOATING:
            //return build_number(node->node_data.float_expr->val);
        case AST_INTEGER:
            return build_i32(node->ival);
        case AST_FLOAT:
            return build_f32(node->fval);
        //case AST_BOOLEAN:
        //    return build_number(node->node_data.bool_expr->val);
        case AST_GREATER_THAN...AST_MOD:
            return ast_node_llvm_binop(node, builder, symbols);
        case AST_ASSIGNMENT:
            return ast_node_llvm_assign(node, builder, symbols);
        case AST_DEFINITION:
            return ast_node_llvm_definition(node, builder, symbols);
        case AST_ARRAY_DEFINITION:
            return ast_node_llvm_array_definition(node, builder, symbols);
        case AST_ARRAY_ACCESS:
            return ast_node_array_access(node, builder, symbols);
        case AST_IF:
            return ast_node_llvm_if(node, builder, symbols);
        case AST_WHILE:
            return ast_node_llvm_while(node, builder, symbols);
        case AST_DEREFERENCE:
            return ast_node_llvm_dereference(node, builder, symbols);
        case AST_CAST:
            return ast_node_llvm_cast(node, builder, symbols);
        case AST_POST_INC ... AST_PRE_DEC:
            return ast_node_llvm_uniop(node, builder, symbols);
        case AST_BREAK:
            if(!current_break_to){
                printf("Break in invalid location\n");
                exit(1);
            }
            return LLVMBuildBr (builder, current_break_to);
        default:
            printf("Error: evaluating ast: unimplemented node type %d\n",node->type);
            exit(1);
    }
}
void generate_obj_file(const char* filename, LLVMModuleRef module) {
    LLVMInitializeAllTargetInfos();
    LLVMInitializeAllTargets();
    LLVMInitializeAllTargetMCs();
    LLVMInitializeAllAsmParsers();
    LLVMInitializeAllAsmPrinters();

    char* triple = LLVMGetDefaultTargetTriple();

    char* error = NULL;
    LLVMTargetRef target = NULL;
    LLVMGetTargetFromTriple(triple, &target, &error);
    if (error) {
        fprintf(stderr, "Error: %s\n", error);
        abort();
    }

    char* cpu = LLVMGetHostCPUName();
    char* features = LLVMGetHostCPUFeatures();
    LLVMTargetMachineRef machine = LLVMCreateTargetMachine(
            target,
            triple,
            cpu,
            features,
            LLVMCodeGenLevelNone,
            LLVMRelocDefault,
            LLVMCodeModelDefault
    );


    LLVMTargetDataRef data_layout = LLVMCreateTargetDataLayout(machine);
    char* data_layout_str = LLVMCopyStringRepOfTargetData(data_layout);

    LLVMSetTarget(module, triple);
    LLVMSetDataLayout(module, data_layout_str);


    LLVMTargetMachineEmitToFile(
            machine,
            module,
            filename,
            LLVMObjectFile,
            &error
    );
    if (error) {
        fprintf(stderr, "Error: %s\n", error);
        abort();
    }

    LLVMDisposeTargetData(data_layout);
    LLVMDisposeMessage(data_layout_str);
    LLVMDisposeTargetMachine(machine);
    LLVMDisposeMessage(cpu);
    LLVMDisposeMessage(features);
    LLVMDisposeMessage(triple);
}

LLVMModuleRef generate_code(ast_node* root, const char* obj){
    //init scope tracker
    scope* symbols=scope_create(NULL);
    //create module
    module = LLVMModuleCreateWithName("LLVMJIT");
    //create initial builder
    LLVMBuilderRef builder = LLVMCreateBuilder();
    //process ast
    ast_node_llvm_statements(root, builder, symbols);

    char* out = LLVMPrintModuleToString(module);
    printf("%s\n", out);

    //check for issues
    LLVMVerifyModule(module, LLVMAbortProcessAction, NULL);
    //optimizations
    LLVMPassManagerRef pass = LLVMCreatePassManager();

    // Add the optimization pass to the pass manager
    //LLVMAddCFGSimplificationPass(pass);
    //LLVMAddGVNPass(pass);
    //LLVMAddDeadStoreEliminationPass(pass);

    // Run the optimization pass on the LLVM IR
    LLVMRunPassManager(pass, module);
    LLVMDisposePassManager(pass);

    //char* out = LLVMPrintModuleToString(module);
    //printf("%s\n", out);


    //if(obj){
        printf("Generating object file\n");
        generate_obj_file("obj.o", module);
    //}

    LLVMDisposeBuilder(builder);
    //LLVMDisposeModule(module);

    return module;
}

