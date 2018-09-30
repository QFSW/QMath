#include "QMath.h"
#include <sstream>
#include <algorithm>
#include <stdlib.h>
#include <iostream>
#include <cctype>
#include <typeinfo>
#ifndef M_E
#define M_E 2.7182818284590452353602874
#endif


using namespace QMath;


bool QMath::isNumber(const std::string& numString)
{
    char* endPtr = nullptr;
    const char* strPtr = numString.c_str();
    strtold(strPtr, &endPtr);
    if(*endPtr != '\0' || endPtr == strPtr) { return false; }
    else { return true; }
}

bool Expression::operator!= (const Expression &b) { return !(*this == b); }

Expression::~Expression() {}

bool Expression::isAtomic() { return false; }

Expression* Expression::simplify() { return copyTree(); }

bool Expression::isCommutative() { return true; }

void Expression::substitute(char var, double value)
{
	std::map<char, double> varMap = std::map<char, double>();
	varMap.emplace(var, value);
	substitute(varMap);
}

Expression* Expression::parse(const std::string& input, bool validateAndRectify)
{
    std::string parseInput = cleanseParseInput(input);
    if (parseInput.size() > 1 && !isNumber(parseInput))
    {
        const ExpressionData table[] = { ExpressionData("+", ExpressionData::Type::Operator),
										 ExpressionData("-", ExpressionData::Type::Operator),
										 ExpressionData("*", ExpressionData::Type::Operator),
										 ExpressionData("/", ExpressionData::Type::Operator),
										 ExpressionData("^", ExpressionData::Type::Operator),
										 ExpressionData("sinc", ExpressionData::Type::Function),
										 ExpressionData("cosec", ExpressionData::Type::Function),
										 ExpressionData("sec", ExpressionData::Type::Function),
										 ExpressionData("csc", ExpressionData::Type::Function),
										 ExpressionData("cotan", ExpressionData::Type::Function),
										 ExpressionData("cot", ExpressionData::Type::Function),
										 ExpressionData("sin", ExpressionData::Type::Function),
										 ExpressionData("cos", ExpressionData::Type::Function),
										 ExpressionData("tan", ExpressionData::Type::Function),
										 ExpressionData("ln", ExpressionData::Type::Function),
										 ExpressionData("log", ExpressionData::Type::Function)};
            
		if (validateAndRectify)
		{
			for (int i = parseInput.size() - 1; i >= 0; --i)
			{
				if (parseInput[i] == '(' && i > 0)
				{
					if (parseInput[i - 1] != '(')
					{
						bool operatorFound = false;
						int offset = 0;
						for (int j = 0; j < sizeof(table) / sizeof(table[0]); ++j)
						{
							int searchSize = table[j].name.size();
							int searchOffset = i - searchSize;
							if (searchOffset >= 0)
							{
								if (parseInput.substr(searchOffset, searchSize) == table[j].name)
								{
									if (table[j].type == ExpressionData::Operator)
									{
										operatorFound = true;
										offset = 0;
										j = sizeof(table) / sizeof(table[0]);
										break;
									}
									else
									{
										offset = -searchSize;
										for (int k = 0; k < sizeof(table) / sizeof(table[0]); ++k)
										{
											int searchIndex = i - searchSize - 1;
											bool validSearch = searchIndex >= 0 && searchIndex < parseInput.size();
											if (validSearch && table[k].type == ExpressionData::Operator && parseInput[searchIndex] == table[k].name[0])
											{
												operatorFound = true;
												k = j = sizeof(table) / sizeof(table[0]);
												break;
											}
										}
									}
								}
							}
						}
						int insertionIndex = i + offset;
						if (!operatorFound)
						{
							if (insertionIndex > 0 && insertionIndex < parseInput.size())
							{
								if (parseInput[insertionIndex - 1] != '(')
								{
									parseInput.insert(insertionIndex, "*");
								}
							}
						}
					}
				}
			}

			for (int i = 0; i < parseInput.size(); ++i)
			{
				if (parseInput[i] == ')' && i < input.size() - 1)
				{
					if (parseInput[i + 1] != ')')
					{
						bool operatorFound = false;
						for (int j = 0; j < sizeof(table) / sizeof(table[0]); ++j)
						{
							if (table[j].type == ExpressionData::Operator && parseInput[i + 1] == table[j].name[0])
							{
								operatorFound = true;
								j = sizeof(table) / sizeof(table[0]);
								break;
							}
						}
						if (!operatorFound && i + 1 < parseInput.size()) { parseInput.insert(i + 1, "*"); }
					}
				}
			}
		}

        for (int i = 0; i < sizeof(table) / sizeof(table[0]); ++i)
        {
            size_t index = -1;
            do
            {
				index = parseInput.find(table[i].name, index + 1);
				if (index != std::string::npos && determineScopeDepth(parseInput, (int)index) == 0)
				{
					switch (table[i].type)
					{
						case ExpressionData::Type::Operator:
						{
							std::string left = parseInput.substr(0, index);
							std::string right = parseInput.substr(index + table[i].name.size(), parseInput.size() - (index + table[i].name.size()));
							return parseOperator(left, right, table[i]);
						}
                        
						case ExpressionData::Type::Function:
						{
							std::string right = parseInput.substr(index + table[i].name.size(), parseInput.size() - (index + table[i].name.size()));
							return parseFunction(right, table[i]);
						}
					}
				}
            } while (index != std::string::npos);
        }
        
        return parseBaseCase(parseInput);
    }
    else { return parseBaseCase(parseInput); }
}

Expression* Expression::parseBaseCase(const std::string &input)
{
    if (isNumber(input)) { return new Number(std::stod(input)); }
    else
    {
        if (input.size() == 1)
        {
            if (input[0] == 'e') { return new Constant('e'); }
            else { return new Variable(input[0]); }
        }
        else
        {
            Expression *leftOperand = parse(input.substr(0, input.size() - 1), false);
            Expression *rightOperand = parse(input.substr(input.size() - 1, 1), false);
            return new Multiply(leftOperand, rightOperand);
        }
    }
}

Expression* Expression::parseOperator(const std::string &inputLeft, const std::string &inputRight, const ExpressionData& operatorData)
{
    Expression* leftOperand = parse(inputLeft, false);
    Expression* rightOperand = parse(inputRight, false);
    
    if (operatorData.name == "+") { return new Add(leftOperand, rightOperand); }
    else if (operatorData.name == "-") { return new Subtract(leftOperand, rightOperand); }
    else if (operatorData.name == "*") { return new Multiply(leftOperand, rightOperand); }
    else if (operatorData.name == "/") { return new Divide(leftOperand, rightOperand); }
    else if (operatorData.name == "^") { return new Exponent(leftOperand, rightOperand); }
    else { throw "Not implemented"; }
}

Expression* Expression::parseFunction(const std::string &inputRight, const ExpressionData& operatorData)
{
    int scopeDepth = 0;
    int scopeEnd = 0;
    do
    {
        if (inputRight[scopeEnd] == '(') { scopeDepth++; }
        else if (inputRight[scopeEnd] == ')') { scopeDepth--; }
        ++scopeEnd;
    } while (scopeDepth != 0);
    
    Expression *operand = parse(inputRight.substr(0, scopeEnd), false);
    
    if (operatorData.name == "sin") { return new Sin(operand); }
    else if (operatorData.name == "cos") { return new Cos(operand); }
    else if (operatorData.name == "tan") { return new Tan(operand); }
    else if (operatorData.name == "sec") { return new Exponent(new Cos(operand), new Number(-1)); }
    else if (operatorData.name == "cosec") { return new Exponent(new Sin(operand), new Number(-1)); }
	else if (operatorData.name == "csc") { return new Exponent(new Sin(operand), new Number(-1)); }
    else if (operatorData.name == "cotan") { return new Exponent(new Tan(operand), new Number(-1)); }
	else if (operatorData.name == "cot") { return new Exponent(new Tan(operand), new Number(-1)); }
    else if (operatorData.name == "sinc") { return new Divide(new Sin(operand), operand->copyTree()); }
    else if (operatorData.name == "ln") { return new Log(new Constant('e'), operand); }
    else if (operatorData.name == "log") { return new Log(new Number(10), operand); }
    else { throw "Not implemented"; }
}

std::string Expression::cleanseParseInput(std::string input)
{
    std::string cleansedInput = input;
    cleansedInput.erase(std::remove_if(cleansedInput.begin(), cleansedInput.end(), static_cast<int(&)(int)>(std::isspace)), cleansedInput.end());
    
    bool reduceScope = false;
    do
    {
        int currentScope = 0;
        int i = 0;
        bool reductionPossible = false;
        do
        {
            if (cleansedInput[i] == '(')
            {
                reductionPossible = true;
                currentScope++;
            }
            else if (cleansedInput[i] == ')')
            {
                reductionPossible = true;
                currentScope--;
            }
            ++i;
            
        } while (currentScope != 0 && i < cleansedInput.size());
        
        reduceScope = i == cleansedInput.size() && reductionPossible;
        if (reduceScope) { cleansedInput = cleansedInput.substr(1, cleansedInput.size() - 2); }
    } while (reduceScope);
    return cleansedInput;
}

unsigned char Expression::determineScopeDepth(const std::string &input, int expansionCenter)
{
    int scopeDepth = 0;
    for (int i = 0; i < expansionCenter; ++i)
    {
        if (input[i] == '(') { scopeDepth++; }
        else if (input[i] == ')') { scopeDepth--; }
    }
    return scopeDepth;
}

Expression::ExpressionData::ExpressionData(std::string name, Type type)
{
    this->name = name;
    this->type = type;
}

bool Operator::operator== (const Expression &b)
{
	if (this == &b) { return true; }
	if (typeid(*this) != typeid(b)) { return false; }
	else
	{
		Operator* bOperator = (Operator*)&b;
		if (isCommutative()) { return (*leftOperand == *bOperator->leftOperand && *rightOperand == *bOperator->rightOperand) ||
										(*leftOperand == *bOperator->rightOperand && *rightOperand == *bOperator->leftOperand); }
		else { return *leftOperand == *bOperator->leftOperand && *rightOperand == *bOperator->rightOperand; }
	}
}

void Operator::determineParentheses(bool& left, bool& right)
{
	if (leftOperand->precedence() < precedence()) { left = true; }
	else { left = false; }
	if (rightOperand->precedence() < precedence()) { right = true; }
	else if (rightOperand->precedence() == precedence() && !rightOperand->isCommutative()) { right = true; }
	else { right = false; }
}

void Operator::toStringOperands(std::string& left, std::string& right)
{
	bool leftParentheses;
	bool rightParentheses;
	determineParentheses(leftParentheses, rightParentheses);
	left = leftOperand->toString(leftParentheses);
	right = rightOperand->toString(rightParentheses);
}

bool Operator::isConstant() { return leftOperand->isConstant() && rightOperand->isConstant(); }

void Operator::substitute(const std::map<char, double>& varMap)
{
	leftOperand->substitute(varMap);
	rightOperand->substitute(varMap);
}

Operator::Operator(Expression *left, Expression *right)
{
	leftOperand = left;
	rightOperand = right;
}

Operator::Operator(double left, Expression *right) : Operator::Operator(new Number(left), right) {}
Operator::Operator(Expression *left, double right) : Operator::Operator(left, new Number(right)) {}
Operator::Operator(double left, double right) : Operator::Operator(new Number(left), new Number(right)) {}

Operator::~Operator()
{
	delete leftOperand;
	delete rightOperand;
}

Expression* Operator::getLeftOperand() { return leftOperand; }
Expression* Operator::getRightOperand() { return rightOperand; }

template<typename T>
Expression* Operator::factoriseLinear(Expression *left, Expression *right)
{
	if (typeid(*left) == typeid(Multiply) || typeid(*right) == typeid(Multiply))
	{
		Multiply* leftMul = typeid(*left) == typeid(Multiply) ? (Multiply*)left->copyTree() : new Multiply(left->copyTree(), 1);
		Multiply* rightMul = typeid(*right) == typeid(Multiply) ? (Multiply*)right->copyTree() : new Multiply(right->copyTree(), 1);
		Expression* leftOperands[2] = { leftMul->getLeftOperand(), leftMul->getRightOperand() };
		Expression* rightOperands[2] = { rightMul->getLeftOperand(), rightMul->getRightOperand() };
		for (int i = 0; i < 2; i++)
		{
			for (int j = 0; j < 2; j++)
			{
				if (*(leftOperands[i]) == *(rightOperands[j]))
				{

					T* newMul = new T(leftOperands[1 - i]->copyTree(), rightOperands[1 - j]->copyTree());
					Expression* newMulSim = newMul->simplify();

					Multiply* result = new Multiply(newMulSim, leftOperands[i]->copyTree());
					Expression* resultSim = result->simplify();

					delete newMul;
					delete result;
					delete leftMul;
					delete rightMul;

					return resultSim;
				}
			}
		}

		delete leftMul;
		delete rightMul;
	}

	return new T(left->copyTree(), right->copyTree());
}

template<typename T1, typename T2>
Expression* Operator::accumulateExponentIndicies(Expression *left, Expression *right)
{
	if (typeid(*left) == typeid(Exponent) || typeid(*right) == typeid(Exponent))
	{
		Exponent* leftExp = typeid(*left) == typeid(Exponent) ? (Exponent*)left->copyTree() : new Exponent(left->copyTree(), 1);
		Exponent* rightExp = typeid(*right) == typeid(Exponent) ? (Exponent*)right->copyTree() : new Exponent(right->copyTree(), 1);
		if (*(leftExp->getLeftOperand()) == *(rightExp->getLeftOperand()))
		{

			T2* newExp = new T2(leftExp->getRightOperand()->copyTree(), rightExp->getRightOperand()->copyTree());
			Expression* newExpSim = newExp->simplify();

			Exponent* result = new Exponent(leftExp->getLeftOperand()->copyTree(), newExpSim);
			Expression* resultSim = result->simplify();

			delete newExp;
			delete result;
			delete leftExp;
			delete rightExp;

			return resultSim;
		}

		delete leftExp;
		delete rightExp;
	}

	return new T1(left->copyTree(), right->copyTree());
}


double Add::evaluate() { return leftOperand->evaluate() + rightOperand->evaluate(); }

Add* Add::differentiate(char diffOperator) { return new Add(leftOperand->differentiate(diffOperator), rightOperand->differentiate(diffOperator)); }

Add* Add::copyTree() { return new Add(leftOperand->copyTree(), rightOperand->copyTree()); }

std::string Add::toString(bool showParentheses)
{
	std::string out = leftOperand->toString() + " + " + rightOperand->toString();
	if (showParentheses) { out = "(" + out + ")"; }
	return out;
}

Expression* Add::simplify()
{
	if (leftOperand->isConstant() && rightOperand->isConstant()) { return new Number(evaluate()); }
	else
	{
		Expression* leftSim = leftOperand->simplify();
		Expression* rightSim = rightOperand->simplify();

		if (leftSim->isConstant() && leftSim->evaluate() == 0)
		{
			delete leftSim;
			return rightSim;
		}
		else if (rightSim->isConstant() && rightSim->evaluate() == 0)
		{
			delete rightSim;
			return leftSim;
		}

		if (*leftSim == *rightSim)
		{
			delete rightSim;
			Multiply* mul = new Multiply(2, leftSim);
			Expression* mulSimplified = mul->simplify();
			delete mul;
			return mulSimplified;
		}

		Expression* result = factoriseLinear<Add>(leftSim, rightSim);
		delete leftSim;
		delete rightSim;
		return result;
	}
}

unsigned char Add::precedence() { return 1; }

Subtract* Subtract::copyTree() { return new Subtract(leftOperand->copyTree(), rightOperand->copyTree()); }

double Subtract::evaluate() { return leftOperand->evaluate() - rightOperand->evaluate(); }

Subtract* Subtract::differentiate(char diffOperator) { return new Subtract(leftOperand->differentiate(diffOperator), rightOperand->differentiate(diffOperator)); }

std::string Subtract::toString(bool showParentheses)
{
	std::string left, right;
	toStringOperands(left, right);
	std::string out = left + " - " + right;
	if (showParentheses) { out = "(" + out + ")"; }
	return out;
}

Expression* Subtract::simplify()
{
	if (leftOperand->isConstant() && rightOperand->isConstant()) { return new Number(evaluate()); }
	else
	{
		Expression* leftSim = leftOperand->simplify();
		Expression* rightSim = rightOperand->simplify();

		if (leftSim->isConstant() && leftSim->evaluate() == 0)
		{
			delete leftSim;
			return new Multiply(-1, rightSim);
		}
		else if (rightSim->isConstant() && rightSim->evaluate() == 0)
		{
			delete rightSim;
			return leftSim;
		}

		if (*leftSim == *rightSim)
		{
			delete leftSim;
			delete rightSim;
			return new Number(0);
		}

		Expression* result = factoriseLinear<Subtract>(leftSim, rightSim);
		delete leftSim;
		delete rightSim;
		return result;
	}
}

unsigned char Subtract::precedence() { return 1; }

bool Subtract::isCommutative() { return false; }


Multiply* Multiply::copyTree() { return new Multiply(leftOperand->copyTree(), rightOperand->copyTree()); }

double Multiply::evaluate() { return leftOperand->evaluate() * rightOperand->evaluate(); }

Add* Multiply::differentiate(char diffOperator)
{
	Multiply *left = new Multiply(leftOperand->copyTree(), rightOperand->differentiate(diffOperator));
	Multiply *right = new Multiply(leftOperand->differentiate(diffOperator), rightOperand->copyTree());
	return new Add(left, right);
}

bool Multiply::isAtomic()
{
    if (leftOperand->isAtomic() && rightOperand->isAtomic())
    {
        if (leftOperand->isConstant()) { return false; }
        else { return true; }
    }
    else { return false; }
}

std::string Multiply::toString(bool showParentheses)
{
	std::string left, right;
	toStringOperands(left, right);
	if (leftOperand->isAtomic() && rightOperand->isAtomic())
	{
		if (!leftOperand->isConstant() && rightOperand->isConstant())
		{
			Expression* tmp = leftOperand;
			leftOperand = rightOperand;
			rightOperand = tmp;
			return right + left;
		}
		else if (leftOperand->isConstant() && rightOperand->isConstant()) { return left + " * " + right; }
		else { return left + right; }
	}
	if (showParentheses) { return "(" + left + " * " + right + ")"; }
	else { return left + " * " + right; }
}

Expression* Multiply::simplify()
{
	if (leftOperand->isConstant() && rightOperand->isConstant()) { return new Number(evaluate()); }
	else
	{
		Expression* leftSim = leftOperand->simplify();
		Expression* rightSim = rightOperand->simplify();
		if (leftSim->isConstant())
		{
			if (leftSim->evaluate() == 0)
            {
                delete leftSim;
                delete rightSim;
                return new Number(0);
            }
			else if (leftSim->evaluate() == 1)
            {
                delete leftSim;
                return rightSim;
            }
		}
        if (rightSim->isConstant())
        {
            if (rightSim->evaluate() == 0)
            {
                delete leftSim;
                delete rightSim;
                return new Number(0);
            }
            else if (rightSim->evaluate() == 1)
            {
                delete rightSim;
                return leftSim;
            }
        }
		
		if (*leftSim == *rightSim)
		{
			delete rightSim;
			return new Exponent(leftSim, 2);
		}

		Expression* result = accumulateExponentIndicies<Multiply, Add>(leftSim, rightSim);
		delete leftSim;
		delete rightSim;
		return result;
	}
}

unsigned char Multiply::precedence() { return 2; }


Divide* Divide::copyTree() { return new Divide(leftOperand->copyTree(), rightOperand->copyTree()); }

double Divide::evaluate() { return leftOperand->evaluate() / rightOperand->evaluate(); }

Divide* Divide::differentiate(char diffOperator)
{
	Multiply *left = new Multiply(leftOperand->differentiate(diffOperator), rightOperand->copyTree());
	Multiply *right = new Multiply(leftOperand->copyTree(), rightOperand->differentiate(diffOperator));
	Subtract *numerator = new Subtract(left, right);
	Exponent *denominator = new Exponent(rightOperand->copyTree(), new Number(2));
	return new Divide(numerator, denominator);
}

std::string Divide::toString(bool showParentheses)
{
	std::string left, right;
	toStringOperands(left, right);
	if (showParentheses) { return "(" + left + " / " + right + ")"; }
	else { return left + " / " + right; }
}

Expression* Divide::simplify()
{
	if (leftOperand->isConstant() && rightOperand->isConstant()) { return new Number(evaluate()); }
	else
	{
		Expression* leftSim = leftOperand->simplify();
		Expression* rightSim = rightOperand->simplify();

		if (leftSim->isConstant() && rightSim->evaluate() == 0)
		{
			delete leftSim;
			delete rightSim;
			return new Number(0);
		}
		if (leftSim->isConstant() && rightSim->evaluate() == 1)
		{
			delete rightSim;
			return leftSim;
		}
		else if (*leftSim == *rightSim)
		{
			delete leftSim;
			delete rightSim;
			return new Number(1);
		}
		
		Expression* result = accumulateExponentIndicies<Divide, Subtract>(leftSim, rightSim);
		delete leftSim;
		delete rightSim;
		return result;
	}
}

unsigned char Divide::precedence() { return 2; }

bool Divide::isCommutative() { return false; }


Exponent* Exponent::copyTree() { return new Exponent(leftOperand->copyTree(), rightOperand->copyTree()); }

double Exponent::evaluate() { return std::pow(leftOperand->evaluate(), rightOperand->evaluate()); }

Multiply* Exponent::differentiate(char diffOperator)
{
	bool constIndex = true;
	if (!rightOperand->isConstant())
	{
		Expression* tmp = rightOperand->differentiate(diffOperator);
		constIndex = tmp->isConstant() && tmp->evaluate() == 0;
		delete tmp;
	}
	if (constIndex)
	{
		Subtract *index = new Subtract(rightOperand->copyTree(), new Number(1));
		Exponent *power = new Exponent(leftOperand->copyTree(), index);
		Multiply *multiplicand = new Multiply(rightOperand->copyTree(), leftOperand->differentiate(diffOperator));
		return new Multiply(multiplicand, power);
	}
	else
    {
        bool constBase = true;
        if (!leftOperand->isConstant())
        {
            Expression* tmp = leftOperand->differentiate(diffOperator);
            constBase = tmp->isConstant() && tmp->evaluate() == 0;
            delete tmp;
        }
        
        if (constBase)
        {
            Expression* index = rightOperand->copyTree();
            Expression* power = new Exponent(leftOperand->copyTree(), index);
            Multiply *multiplicand = new Multiply(new Log(new Constant('e'), leftOperand->copyTree()), rightOperand->differentiate(diffOperator));
            return new Multiply(multiplicand, power);
        }
        else
        {
            Multiply* rightTop = new Multiply(rightOperand->copyTree(), leftOperand->differentiate(diffOperator));
            Divide* right = new Divide(rightTop, rightOperand->differentiate(diffOperator));
            Multiply* left = new Multiply(rightOperand->differentiate(diffOperator), new Log(new Constant('e'), rightOperand->copyTree()));
            Add* inner = new Add(left, right);
            return new Multiply(copyTree(), inner);
        }
    }
}

std::string Exponent::toString(bool showParentheses)
{
	std::string left, right;
	toStringOperands(left, right);
	if (showParentheses) { return "(" + left + "^" + right + ")"; }
	else { return left + "^" + right; }
}

Expression* Exponent::simplify()
{
	if (leftOperand->isConstant() && rightOperand->isConstant()) { return new Number(evaluate()); }
	else
	{
		if (leftOperand->isConstant())
		{
			if (leftOperand->evaluate() == 0) { return new Number(0); }
			else if (leftOperand->evaluate() == 1) { return new Number(1); }
		}
		if (rightOperand->isConstant())
		{
			if (rightOperand->evaluate() == 0) { return new Number(1); }
			else if (rightOperand->evaluate() == 1) { return leftOperand->simplify(); }
		}
		return new Exponent(leftOperand->simplify(), rightOperand->simplify());
	}
}

unsigned char Exponent::precedence() { return 3; }

bool Exponent::isCommutative() { return false; }


Log::Log(Expression *left, Expression *right) : Operator::Operator(left, right)
{
    if (left->isConstant() && left->isAtomic() && left->evaluate() == 10) { is10 = true; }
    else
    {
        Expression *tmp = left->differentiate();
        if (tmp->isConstant() && left->isAtomic() && tmp->evaluate() == 0 && left->evaluate() == M_E) { isNatural = true; }
        delete tmp;
    }
}

std::string Log::toString(bool showParentheses)
{
    std::string base;
    if (isNatural) { base = "ln"; }
    else
    {
        base = "log";
        if (!is10) { base += "[" + leftOperand->toString() + "]"; }
    }
    
    return base + "(" + rightOperand->toString() + ")";
}

Log* Log::copyTree() { return new Log(leftOperand->copyTree(), rightOperand->copyTree()); }

double Log::evaluate()
{
    if (isNatural) { return std::log(rightOperand->evaluate()); }
    else if (is10) { return std::log10(rightOperand->evaluate()); }
    else { return std::log(rightOperand->evaluate()) / std::log(leftOperand->evaluate()); }
}

Expression* Log::differentiate(char diffOperator)
{
    if (isNatural) { return new Divide(rightOperand->differentiate(), rightOperand->copyTree()); }
    else
    {
        Expression* tmp = leftOperand->differentiate();
        if (tmp->isConstant() && tmp->evaluate() == 0)
        {
            delete tmp;
            Expression* numerator = rightOperand->differentiate(diffOperator);
            Multiply* denominator = new Multiply(rightOperand->copyTree(), new Log(new Constant('e'), leftOperand->copyTree()));
            return new Divide(numerator, denominator);
        }
        delete tmp;
        
        Multiply* right = new Multiply(copyTree(), new Divide(leftOperand->differentiate(diffOperator), leftOperand->copyTree()));
        Divide* left = new Divide(rightOperand->differentiate(diffOperator), rightOperand->copyTree());
        Subtract* inner = new Subtract(left, right);
        Divide* outer = new Divide(copyTree(), new Log(new Constant('e'), leftOperand->copyTree()));
        return new Multiply(outer, inner);
    }
}

Expression* Log::simplify()
{
    if (isConstant()) { return new Number(evaluate()); }
    else
    {
        bool operandsEqual = true;
        Expression *tmp;
        
        tmp = leftOperand->differentiate();
        operandsEqual &= tmp->isConstant() && tmp->evaluate() == 0;
        delete tmp;
        
        tmp = rightOperand->differentiate();
        operandsEqual &= tmp->isConstant() && tmp->evaluate() == 0;
        delete tmp;
        
        if (operandsEqual) { operandsEqual = leftOperand->evaluate() == rightOperand->evaluate(); }
        if (operandsEqual) { return new Number(1); }
        else { return copyTree(); }
    }
}

unsigned char Log::precedence() { return 10; }
bool Log::isCommutative() { return false; }


bool Number::operator== (const Expression &b)
{
	if (this == &b) { return true; }
	if (typeid(*this) != typeid(b)) { return false; }
	else
	{
		Number* bNumber = (Number*)&b;
		return value == bNumber->value;
	}
}

double Number::evaluate() { return value; }

bool Number::isConstant() { return true; }

Number::Number(double value) { this->value = value; }

Number* Number::differentiate(char diffOperator) { return new Number(0); }

Number* Number::copyTree() { return new Number(value); }

std::string Number::toString(bool showParentheses)
{
	std::stringstream stream;
	stream << value;
	return stream.str();
}

void Number::substitute(const std::map<char, double>& varMap) {}

bool Number::isAtomic() { return true; }

unsigned char Number::precedence() { return 0; }


bool Variable::operator== (const Expression &b)
{
	if (this == &b) { return true; }
	if (typeid(*this) != typeid(b)) { return false; }
	else
	{
		Variable* bVariable = (Variable*)&b;
		return var == bVariable->var;
	}
}

double Variable::evaluate() { return value; }

bool Variable::isConstant() { return false; }

Variable::Variable(char var, double value)
{
	this->var = var;
	this->value = value;
}

Variable::Variable(char var) : Variable::Variable(var, 0) {}

Expression* Variable::differentiate(char diffOperator)
{
	if (diffOperator == var) { return new Number(1); }
	else { return new Differential(copyTree(), new Variable(diffOperator)); }
}

Variable* Variable::copyTree() { return new Variable(var, value); }

std::string Variable::toString(bool showParentheses)
{
	std::stringstream stream;
	stream << var;
	return stream.str();
}

char Variable::charID() { return var; }

bool Variable::isAtomic() { return true; }

void Variable::substitute(const std::map<char, double>& varMap)
{
	if (varMap.find(var) != varMap.end())
	{
		value = varMap.at(var);
	}
}

unsigned char Variable::precedence() { return 0; }


Constant::Constant(char var, double value) : Variable::Variable(var, value)
{
    if (var == 'e')
    {
        this->value = M_E;
    }
}

Constant::Constant(char var) : Constant::Constant(var, 0) {}

Number* Constant::differentiate(char diffOperator) { return new Number(0); }

Constant* Constant::copyTree() { return new Constant(var, value); }


bool Func::operator== (const Expression &b)
{
	if (this == &b) { return true; }
	if (typeid(*this) != typeid(b)) { return false; }
	else
	{
		Func* bFunc = (Func*)&b;
		return *operand == *bFunc->operand;
	}
}

Func::Func(Expression *operand) { this->operand = operand; }

Func::~Func() { delete operand; }

bool Func::isConstant() { return operand->isConstant(); }

bool Func::isAtomic() { return true; }

void Func::substitute(const std::map<char, double>& varMap) { operand->substitute(varMap); }

unsigned char Func::precedence() { return 4; }


Sin* Sin::copyTree() { return new Sin(operand->copyTree()); }

double Sin::evaluate() { return std::sin(operand->evaluate()); }

Multiply* Sin::differentiate(char diffOperator)
{
	Expression* left = operand->differentiate(diffOperator);
	Cos* right = new Cos(operand->copyTree());
	return new Multiply(left, right);
}

Sin* Sin::simplify() { return new Sin(operand->simplify()); }

std::string Sin::toString(bool showParentheses) { return "sin(" + operand->toString() + ")"; }


Cos* Cos::copyTree() { return new Cos(operand->copyTree()); }

double Cos::evaluate() { return std::cos(operand->evaluate()); }

Multiply* Cos::differentiate(char diffOperator)
{
	Subtract* left = new Subtract(new Number(0), operand->differentiate(diffOperator));
	Sin* right = new Sin(operand->copyTree());
	return new Multiply(left, right);
}

Cos* Cos::simplify() { return new Cos(operand->simplify()); }

std::string Cos::toString(bool showParentheses) { return "cos(" + operand->toString() + ")"; }


Tan* Tan::copyTree() { return new Tan(operand->copyTree()); }

double Tan::evaluate() { return std::tan(operand->evaluate()); }

Multiply* Tan::differentiate(char diffOperator)
{
    Expression* left = operand->differentiate(diffOperator);
    Exponent* right = new Exponent(new Cos(operand->copyTree()), new Number(-2));
    return new Multiply(left, right);
}

Tan* Tan::simplify() { return new Tan(operand->simplify()); }

std::string Tan::toString(bool showParentheses) { return "tan(" + operand->toString() + ")"; }


Differential::Differential(Expression *left, Expression *right, unsigned char order) : Operator::Operator(left, right)
{
    this->order = order;
}

Differential* Differential::copyTree()
{
    return new Differential(leftOperand->copyTree(), rightOperand->copyTree(), order);
}

double Differential::evaluate() { throw "Not implemented"; }

Expression* Differential::differentiate(char diffOperator)
{
    Expression* tmp = rightOperand->differentiate(diffOperator);
    bool sameOperator = tmp->isConstant();
    delete tmp;
    
    if (sameOperator) { return new Differential(leftOperand, rightOperand, order + 1); }
    else
    {
        Differential* rhs = new Differential(rightOperand, new Variable(diffOperator));
        Differential* lhs = new Differential(leftOperand, rightOperand, order + 1);
        return new Multiply(lhs, rhs);
    }
}

bool Differential::isCommutative() { return false; }

unsigned char Differential::precedence() { return 2; }

std::string Differential::toString(bool useParentheses)
{
    std::string top = "d";
    if (order > 1) { top += "^" + std::to_string(order); }
    top += leftOperand->toString();
    
    std::string bottom = "d" + rightOperand->toString();
    if (order > 1) { bottom += "^" + std::to_string(order); }
    
    std::string str = top + "/" + bottom;
    if (useParentheses){ str = "(" + str + ")"; }
    return str;
}

