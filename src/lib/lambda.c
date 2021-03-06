#include <slash.h>
#include <stdio.h>

typedef struct {
    sl_object_t base;
    sl_vm_section_t* section;
    sl_vm_exec_ctx_t* parent_ctx;
}
sl_lambda_t;

sl_lambda_t*
get_lambda(sl_vm_t* vm, SLVAL obj)
{
    return (sl_lambda_t*)sl_get_ptr(sl_expect(vm, obj, vm->lib.Lambda));
}
SLVAL
sl_make_lambda(sl_vm_section_t* section, sl_vm_exec_ctx_t* parent_ctx)
{
    SLVAL lambda = sl_allocate(parent_ctx->vm, parent_ctx->vm->lib.Lambda);
    sl_lambda_t* lp = get_lambda(parent_ctx->vm, lambda);
    lp->section = section;
    lp->parent_ctx = parent_ctx;
    return lambda;
}

static sl_object_t*
allocate_lambda(sl_vm_t* vm)
{
    return sl_alloc(vm->arena, sizeof(sl_lambda_t));
}

static SLVAL
apply_lambda(sl_vm_t* vm, sl_lambda_t* lambda, SLVAL self, int argc, SLVAL* argv)
{
    if(argc < lambda->section->arg_registers) {
        sl_error(vm, vm->lib.ArgumentError, "Too few arguments in lambda call. Expected %d, received %d", (int)lambda->section->arg_registers, argc);
    }
    sl_vm_exec_ctx_t* subctx = sl_alloc(vm->arena, sizeof(sl_vm_exec_ctx_t));
    subctx->vm = vm;
    subctx->section = lambda->section;
    subctx->registers = sl_alloc(vm->arena, sizeof(SLVAL) * lambda->section->max_registers);
    for(int i = 0; i < lambda->section->max_registers; i++) {
        subctx->registers[i] = vm->lib.nil;
    }
    subctx->self = self;
    subctx->parent = lambda->parent_ctx;
    for(int i = 0; i < lambda->section->arg_registers; i++) {
        subctx->registers[i + 1] = argv[i];
    }
    return sl_vm_exec(subctx, 0);
}

SLVAL
sl_lambda_call_with_self(sl_vm_t* vm, SLVAL lambda, SLVAL self, size_t argc, SLVAL* argv)
{
    sl_lambda_t* lp = get_lambda(vm, lambda);
    return apply_lambda(vm, lp, self, argc, argv);
}

static SLVAL
sl_lambda_call_with_self2(sl_vm_t* vm, SLVAL lambda, size_t argc, SLVAL* argv)
{
    sl_lambda_t* lp = get_lambda(vm, lambda);
    return apply_lambda(vm, lp, argv[0], argc - 1, argv + 1);
}

SLVAL
sl_lambda_call(sl_vm_t* vm, SLVAL lambda, size_t argc, SLVAL* argv)
{
    sl_lambda_t* lp = get_lambda(vm, lambda);
    return apply_lambda(vm, lp, lp->parent_ctx->self, argc, argv);
}

SLVAL
sl_lambda_to_method(sl_vm_t* vm, SLVAL lambda)
{
    sl_lambda_t* lp = get_lambda(vm, lambda);
    SLID name = sl_intern(vm, "<lambda>");
    return sl_make_method(vm, vm->lib.Object, name, lp->section, lp->parent_ctx);
}

void
sl_init_lambda(sl_vm_t* vm)
{
    vm->lib.Lambda = sl_define_class(vm, "Lambda", vm->lib.Object);
    sl_class_set_allocator(vm, vm->lib.Lambda, allocate_lambda);
    sl_define_method(vm, vm->lib.Lambda, "call", -1, sl_lambda_call);
    sl_define_method(vm, vm->lib.Lambda, "call_with_self", -2, sl_lambda_call_with_self2);
}
