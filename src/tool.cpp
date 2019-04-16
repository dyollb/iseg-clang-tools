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


// makes a matcher for C++ member call expressions in namespace <ns_name>
auto mk_call_expr_matcher(std::string const & class_name)
{
  return cxxMemberCallExpr(hasDeclaration(
        cxxMethodDecl(isPublic(), ofClass(hasName(class_name))).bind("class")
      )).bind("call");
}

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
      std::cout << "Method '" << method_name << "' invoked by object of type '"
                << callee_name << "'\n";
    }
  }  // run

  uint32_t num_calls = 0;
};  // CallPrinter


int main(int argc, const char **argv) {
  CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  CallPrinter cp;

  {
    MatchFinder finder;
    finder.addMatcher(mk_call_expr_matcher("SlicesHandler"), &cp);
    Tool.run(newFrontendActionFactory(&finder).get());
    std::cout << "Reported " << cp.num_calls << " member calls\n";
  }
  {
    MatchFinder finder;
    finder.addMatcher(mk_call_expr_matcher("Transform"), &cp);
    Tool.run(newFrontendActionFactory(&finder).get());
    std::cout << "Reported " << cp.num_calls << " member calls\n";
  }
  return 0;
}
