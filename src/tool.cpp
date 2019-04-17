// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"

#include <iostream>

using namespace clang;
using namespace clang::tooling;
using namespace clang::ast_matchers;
using namespace llvm;

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("my-tool options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nMore help text...\n");

// Some links:
// https://github.com/samanbarghi/CPP2C/blob/master/CPP2C.cpp
// https://clang.llvm.org/doxygen/LLVMConventionsChecker_8cpp_source.html
// https://clang.llvm.org/doxygen/classclang_1_1tooling_1_1FindSymbolOccurrencesRefactoringRule.html
// https://github.com/eliben/llvm-clang-samples/blob/master/src_clang/experimental/apply_replacements_with_rewriter.cpp
// http://clang-developers.42468.n3.nabble.com/any-complete-example-of-a-Rewriter-that-changes-function-call-arguments-td4029085.html
// http://clang.llvm.org/docs/LibASTMatchersTutorial.html


// makes a matcher for C++ member call expressions in class <class_name> in namespace "iseg::"
auto mk_call_expr_matcher(std::string const & class_name)
{
  return cxxMemberCallExpr(hasDeclaration(
        cxxMethodDecl(isPublic(), 
          ofClass(matchesName("iseg::*" + class_name)))
      )).bind("call");
}

auto mk_method_decl_matcher(std::string const & class_name)
{
  return cxxMethodDecl(isPublic(), 
          ofClass(matchesName("iseg::*" + class_name))).bind("method");
}

auto mk_record_match(std::string const & ns_name)
{
  return cxxRecordDecl(matchesName(ns_name)).bind("class");
}

struct CollectClasses : public MatchFinder::MatchCallback {
  void run(MatchFinder::MatchResult const & result) override
  {
    using namespace clang;
    if(auto record = result.Nodes.getNodeAs<CXXRecordDecl>("class")) {
      auto const record_name(record->getNameAsString());
      _classes.push_back(record_name);

      if (record_name != "Transform")
        return;

      for (auto field: record->fields())
      {
        std::cerr << "\tField: " << field->getQualifiedNameAsString() << "\n";

        // \todo how to find where it is declared and used?
        //field->getDeclContext().
      }
      // \todo why does this not get all methods in class?
      for (auto method: record->methods())
      {
        std::cerr << "\tMethod: " << method->getNameAsString() << "\n";
      }
    }
  }  // run

  std::vector<std::string> _classes;
};

/* CallPrinter::run() will be called when a match is found */
struct CallPrinter : public MatchFinder::MatchCallback {
  void run(MatchFinder::MatchResult const & result) override
  {
    using namespace clang;
    //SourceManager & sm(result.Context->getSourceManager());
    if(auto call = result.Nodes.getNodeAs<CXXMemberCallExpr>("call")) {
      num_calls++;
      auto const method_name(call->getMethodDecl()->getNameAsString());
      auto const callee_name(call->getRecordDecl()->getNameAsString());
      std::cout << "Method Call '" << method_name << "' invoked by object of type '" << callee_name << "'\n";
    }
    else if(auto member = result.Nodes.getNodeAs<CXXMethodDecl>("method"))
    {
      auto const method_name(member->getQualifiedNameAsString());
      std::cout << "Method Decl '" << method_name << "'\n";
    }
  }  // run

  uint32_t num_calls = 0;
  uint32_t num_records = 0;
};  // CallPrinter


int main(int argc, const char **argv) {
  CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  // collect classes in namespace
  std::set<std::string> classes;
  {
    CollectClasses cp;
    MatchFinder finder;

    finder.addMatcher(mk_record_match("iseg::"), &cp);
    Tool.run(newFrontendActionFactory(&finder).get());

    classes.insert(cp._classes.begin(), cp._classes.end());
    std::cout << "Reported " << classes.size() << " records\n";
  }

  {
    for (auto class_name: classes)
    {
      if (class_name.empty())
        continue;
      if (class_name != "SlicesHandler")
        continue;
      CallPrinter cp;
      MatchFinder finder;
      finder.addMatcher(mk_call_expr_matcher(class_name), &cp);
      finder.addMatcher(mk_method_decl_matcher(class_name), &cp);
      Tool.run(newFrontendActionFactory(&finder).get());
      std::cout << class_name << "\t: Reported " << cp.num_calls << " member calls\n";

      break;
    }
  }
  return 0;
}
