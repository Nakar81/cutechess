/*
    This file is part of Cute Chess.

    Cute Chess is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Cute Chess is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Cute Chess.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "enginemanager.h"
#include <QFile>
#include <QTextStream>
#include <jsonparser.h>
#include <jsonserializer.h>


EngineManager::EngineManager(QObject* parent)
	: QObject(parent)
{
}

EngineManager::~EngineManager()
{
}

int EngineManager::engineCount() const
{
	return m_engines.count();
}

EngineConfiguration EngineManager::engineAt(int index) const
{
	return m_engines.at(index);
}

int EngineManager::engineIndex(const QString& name) const
{
	int index = 0;
	for (const EngineConfiguration& engine : m_engines)
	{
		if (name == engine.name())
			return index;
		++index;
	}
	return -1;
}

void EngineManager::addEngine(const EngineConfiguration& engine)
{
	m_engines << engine;

	emit engineAdded(m_engines.size() - 1);
}

void EngineManager::updateEngineAt(int index, const EngineConfiguration& engine)
{
	m_engines[index] = engine;

	emit engineUpdated(index);
}

void EngineManager::removeEngineAt(int index)
{
	emit engineAboutToBeRemoved(index);

	m_engines.removeAt(index);
}

QList<EngineConfiguration> EngineManager::engines() const
{
	return m_engines;
}

void EngineManager::setEngines(const QList<EngineConfiguration>& engines)
{
	m_engines = engines;

	emit enginesReset();
}

bool EngineManager::supportsVariant(const QString& variant) const
{
	if (m_engines.isEmpty())
		return false;

	// TODO: use qAsConst() from Qt 5.7
	foreach (const auto& config, m_engines)
	{
		if (!config.supportsVariant(variant))
			return false;
	}

	return true;
}

void EngineManager::loadEngines(const QString& fileName)
{
	if (!QFile::exists(fileName))
		return;

	QFile input(fileName);
	if (!input.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning("cannot open engine configuration file: %s",
			 qUtf8Printable(fileName));
		return;
	}

	QTextStream stream(&input);
	JsonParser parser(stream);
	const QVariantList engines(parser.parse().toList());

	if (parser.hasError())
	{
		qWarning("%s", qUtf8Printable(QString("bad engine configuration file line %1 in %2: %3")
			.arg(parser.errorLineNumber()).arg(fileName).arg(parser.errorString()))); // clazy:exclude=qstring-arg
		return;
	}

	for (const QVariant& engine : engines)
		addEngine(EngineConfiguration(engine));
}

void EngineManager::reloadEngines(const QString& fileName)
{
	if (!QFile::exists(fileName))
		return;

	QFile input(fileName);
	if (!input.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning("cannot open engine configuration file: %s", qPrintable(fileName));
		return;
	}

	QTextStream stream(&input);
	JsonParser parser(stream);
	const QVariantList engines(parser.parse().toList());

	if (parser.hasError())
	{
		qWarning("%s", qPrintable(QString("bad engine configuration file line %1 in %2: %3")
			.arg(parser.errorLineNumber()).arg(fileName).arg(parser.errorString()))); // clazy:exclude=qstring-arg
		return;
	}

	QSet<QString> names = engineNames();
	QList<EngineConfiguration> newEngines;
	for (const QVariant& engine : engines)
		newEngines << engine;

	for (const EngineConfiguration& engine : newEngines)
	{
		const int index = engineIndex(engine.name());
		if (index >= 0)
		{
			names.remove(engine.name());
			if (engineAt(index) != engine)
				updateEngineAt(index, engine);
		}
		else
			addEngine(engine);
	}

	for (const QString& name : names)
	{
		const int index = engineIndex(name);
		if (index >= 0)
			removeEngineAt(index);
	}
}

void EngineManager::saveEngines(const QString& fileName)
{
	QVariantList engines;
	// TODO: use qAsConst() from Qt 5.7
	foreach (const EngineConfiguration& config, m_engines)
		engines << config.toVariant();

	QFile output(fileName);
	if (!output.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		qWarning("cannot open engine configuration file: %s",
			 qUtf8Printable(fileName));
		return;
	}

	QTextStream out(&output);
	JsonSerializer serializer(engines);
	serializer.serialize(out);
}

QSet<QString> EngineManager::engineNames() const
{
	QSet<QString> names;
	// TODO: use qAsConst() from Qt 5.7
	foreach (const EngineConfiguration& engine, m_engines)
		names.insert(engine.name());

	return names;
}
