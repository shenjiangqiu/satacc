#ifndef SET_UP_H
#define SET_UP_H

#include "assign_wrap.h"
auto generate_wrap()
{
    assign_wrap_factory af;
    auto new_wrap1 = af.create(10, 32, -1, nullptr, 0);

    for (int i = 0; i < 32; i++)
    {
        new_wrap1->add_block_addr(i, 200 + i);
    }
    auto new_wrap2 = af.create(11, 16, 2, new_wrap1, 1);
    for (int i = 0; i < 16; i++)
    {
        new_wrap2->add_block_addr(i, 400 + i);
    }
    auto new_wrap3 = af.create(11, 16, 2, new_wrap1, 1);
    for (int i = 0; i < 16; i++)
    {
        new_wrap3->add_block_addr(i, 600 + i);
    }
    new_wrap1->set_addr(3333);
    new_wrap1->add_modified_list(0, 1000);
    new_wrap1->add_modified_list(10, 2000);
    //new_wrap1->add_modified_list(0, 100);

    new_wrap1->add_clause_literal(0, 0);
    new_wrap1->add_clause_literal(0, 2);
    new_wrap1->add_clause_literal(0, 3);
    new_wrap1->add_detail(0, 11);
    new_wrap1->add_detail(0, 12);
    new_wrap1->add_detail(0, 13);
    new_wrap1->add_generated_assignments(10, new_wrap2);
    new_wrap1->add_modified_list(10, 200);
    new_wrap1->add_clause_literal(10, 0);
    new_wrap1->add_clause_literal(10, 2);
    new_wrap1->add_clause_literal(10, 3);
    new_wrap1->add_detail(10, 11);
    new_wrap1->add_detail(10, 12);
    new_wrap1->add_detail(10, 13);

    new_wrap2->set_addr(4444);
    new_wrap2->add_modified_list(0, 200);
    new_wrap2->add_detail(0, 22);
    new_wrap2->add_detail(0, 23);
    new_wrap2->add_detail(0, 24);

    new_wrap2->add_generated_assignments(0, new_wrap3);

    return new_wrap1;
}
auto generate_wrap_short()
{
    assign_wrap_factory af;
    auto new_wrap1 = af.create(10, 32, -1, nullptr, 0);

    for (int i = 0; i < 32; i++)
    {
        new_wrap1->add_block_addr(i, 200 + i);
    }
    auto new_wrap2 = af.create(11, 16, 2, new_wrap1, 1);
    for (int i = 0; i < 16; i++)
    {
        new_wrap2->add_block_addr(i, 400 + i);
    }
    auto new_wrap3 = af.create(11, 16, 2, new_wrap1, 1);
    for (int i = 0; i < 16; i++)
    {
        new_wrap3->add_block_addr(i, 600 + i);
    }
    new_wrap1->set_addr(3333);
    new_wrap1->add_modified_list(0, 1000);
    new_wrap1->add_modified_list(10, 2000);
    //new_wrap1->add_modified_list(0, 100);

    new_wrap1->add_clause_literal(0, 0);
    new_wrap1->add_clause_literal(0, 2);
    new_wrap1->add_clause_literal(0, 3);
    new_wrap1->add_detail(0, 11);
    new_wrap1->add_detail(0, 12);
    new_wrap1->add_detail(0, 13);
    //new_wrap1->add_generated_assignments(10, new_wrap2);
    new_wrap1->add_modified_list(10, 200);
    new_wrap1->add_clause_literal(10, 0);
    new_wrap1->add_clause_literal(10, 2);
    new_wrap1->add_clause_literal(10, 3);
    new_wrap1->add_detail(10, 11);
    new_wrap1->add_detail(10, 12);
    new_wrap1->add_detail(10, 13);

    new_wrap2->set_addr(4444);
    new_wrap2->add_modified_list(0, 200);
    new_wrap2->add_detail(0, 22);
    new_wrap2->add_detail(0, 23);
    new_wrap2->add_detail(0, 24);

    //  new_wrap2->add_generated_assignments(0, new_wrap3);

    return new_wrap1;
}
auto generate_wrap_para()
{
    assign_wrap_factory af;
    auto new_wrap1 = af.create(10, 32, -1, nullptr, 0);

    for (int i = 0; i < 32; i++)
    {
        new_wrap1->add_block_addr(i, 200 + i);
    }
    auto new_wrap2 = af.create(11, 16, 2, new_wrap1, 1);
    for (int i = 0; i < 16; i++)
    {
        new_wrap2->add_block_addr(i, 400 + i);
    }
    new_wrap2->set_addr(111);
    auto new_wrap3 = af.create(11, 16, 2, new_wrap1, 1);
    for (int i = 0; i < 16; i++)
    {
        new_wrap3->add_block_addr(i, 600 + i);
    }
    new_wrap3->set_addr(10);
    auto new_wrap4 = af.create(11, 16, 2, new_wrap1, 1);
    for (int i = 0; i < 16; i++)
    {
        new_wrap4->add_block_addr(i, 600 + i);
    }

    new_wrap4->set_addr(2222);
    new_wrap1->set_addr(3333);

    new_wrap1->add_modified_list(0, 1000);
    new_wrap1->add_modified_list(10, 2000);
    //new_wrap1->add_modified_list(0, 100);

    new_wrap1->add_clause_literal(0, 0);
    new_wrap1->add_clause_literal(0, 2);
    new_wrap1->add_clause_literal(0, 3);
    new_wrap1->add_detail(0, 11);
    new_wrap1->add_detail(0, 12);
    new_wrap1->add_detail(0, 13);
    new_wrap1->add_generated_assignments(10, new_wrap2);
    new_wrap1->add_modified_list(10, 200);
    new_wrap1->add_clause_literal(10, 0);
    new_wrap1->add_clause_literal(10, 2);
    new_wrap1->add_clause_literal(10, 3);
    new_wrap1->add_detail(10, 11);
    new_wrap1->add_detail(10, 12);
    new_wrap1->add_detail(10, 13);

    new_wrap2->set_addr(4444);
    new_wrap2->add_modified_list(0, 200);
    new_wrap2->add_detail(0, 22);
    new_wrap2->add_detail(0, 23);
    new_wrap2->add_detail(0, 24);

    new_wrap2->add_generated_assignments(0, new_wrap3);

    return std::make_pair(new_wrap1, new_wrap4);
}

#endif