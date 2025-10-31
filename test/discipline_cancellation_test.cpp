/*
    Philote C++ Bindings

    Copyright 2022-2025 Christopher A. Lupp

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <gtest/gtest.h>
#include "explicit.h"
#include "test_helpers.h"

using namespace philote;
using namespace philote::test;

// ============================================================================
// Discipline Cancellation Unit Tests
// ============================================================================

class DisciplineCancellationTest : public ::testing::Test {
protected:
    void SetUp() override {
        discipline_ = std::make_shared<ParaboloidDiscipline>();
    }

    std::shared_ptr<ParaboloidDiscipline> discipline_;
};

TEST_F(DisciplineCancellationTest, IsCancelledWithNullContext) {
    // Without setting context, IsCancelled should return false
    EXPECT_FALSE(discipline_->IsCancelled());
}

TEST_F(DisciplineCancellationTest, SetContextStoresPointer) {
    // Initially should not be cancelled
    EXPECT_FALSE(discipline_->IsCancelled());

    // Create a real ServerContext (won't be cancelled)
    grpc::ServerContext context;
    discipline_->SetContext(&context);

    // Context is now set, IsCancelled queries the real context
    // Real contexts are not cancelled unless the client actually cancels
    EXPECT_FALSE(discipline_->IsCancelled());
}

TEST_F(DisciplineCancellationTest, ClearContextRemovesPointer) {
    grpc::ServerContext context;
    discipline_->SetContext(&context);
    discipline_->ClearContext();

    // After clearing, IsCancelled should return false (no context)
    EXPECT_FALSE(discipline_->IsCancelled());
}

TEST_F(DisciplineCancellationTest, MultipleSetClearCycles) {
    grpc::ServerContext context1;

    // First cycle
    discipline_->SetContext(&context1);
    EXPECT_FALSE(discipline_->IsCancelled());

    discipline_->ClearContext();
    EXPECT_FALSE(discipline_->IsCancelled());

    // Second cycle with new context
    grpc::ServerContext context2;
    discipline_->SetContext(&context2);
    EXPECT_FALSE(discipline_->IsCancelled());

    discipline_->ClearContext();
    EXPECT_FALSE(discipline_->IsCancelled());
}

TEST_F(DisciplineCancellationTest, ConstCorrectnessOfMethods) {
    grpc::ServerContext context;

    // Verify that SetContext and ClearContext work with const discipline pointer
    const Discipline* const_discipline = discipline_.get();

    // These should compile and work even with const pointer
    const_discipline->SetContext(&context);
    const_discipline->ClearContext();
    const_discipline->IsCancelled();
}

TEST_F(DisciplineCancellationTest, ContextPropagationInSlowDiscipline) {
    // SlowDiscipline checks IsCancelled() internally
    auto slow_discipline = std::make_shared<SlowDiscipline>(50);

    // Without context, should complete normally
    Variables inputs;
    inputs["x"] = Variable(kInput, {1});
    inputs["x"](0) = 1.0;

    Variables outputs;
    outputs["y"] = Variable(kOutput, {1});

    // Should not throw - completes normally
    EXPECT_NO_THROW(slow_discipline->Compute(inputs, outputs));
    EXPECT_FALSE(slow_discipline->WasCancelled());
    EXPECT_DOUBLE_EQ(outputs["y"](0), 2.0);
}
