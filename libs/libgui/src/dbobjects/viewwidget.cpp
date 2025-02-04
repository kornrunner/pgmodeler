/*
# PostgreSQL Database Modeler (pgModeler)
#
# Copyright 2006-2023 - Raphael Araújo e Silva <raphael@pgmodeler.io>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation version 3.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# The complete text of GPLv3 is at LICENSE file on source code root directory.
# Also, you can get the complete GNU General Public License at <http://www.gnu.org/licenses/>
*/

#include "viewwidget.h"
#include "rulewidget.h"
#include "triggerwidget.h"
#include "indexwidget.h"
#include "baseform.h"
#include "referencewidget.h"
#include "coreutilsns.h"

ViewWidget::ViewWidget(QWidget *parent): BaseObjectWidget(parent, ObjectType::View)
{
	try
	{
		ObjectsTableWidget *tab=nullptr;
		ObjectType types[]={ ObjectType::Trigger, ObjectType::Rule, ObjectType::Index };
		QGridLayout *grid=nullptr;
		QVBoxLayout *vbox=nullptr;

		Ui_ViewWidget::setupUi(this);

		code_txt=new NumberedTextEditor(this);
		code_txt->setReadOnly(true);
		code_hl=new SyntaxHighlighter(code_txt);
		code_hl->loadConfiguration(GlobalAttributes::getSQLHighlightConfPath());
		vbox=new QVBoxLayout(code_prev_tab);
		vbox->setContentsMargins(GuiUtilsNs::LtMargin,GuiUtilsNs::LtMargin,GuiUtilsNs::LtMargin,GuiUtilsNs::LtMargin);
		vbox->addWidget(code_txt);

		cte_expression_txt=new NumberedTextEditor(this, true);
		cte_expression_hl=new SyntaxHighlighter(cte_expression_txt);
		cte_expression_hl->loadConfiguration(GlobalAttributes::getSQLHighlightConfPath());
		vbox=new QVBoxLayout(cte_tab);
		vbox->setContentsMargins(GuiUtilsNs::LtMargin,GuiUtilsNs::LtMargin,GuiUtilsNs::LtMargin,GuiUtilsNs::LtMargin);
		vbox->addWidget(cte_expression_txt);

		tag_sel=new ObjectSelectorWidget(ObjectType::Tag, this);
		dynamic_cast<QGridLayout *>(options_gb->layout())->addWidget(tag_sel, 0, 1, 1, 4);

		references_tab=new ObjectsTableWidget(ObjectsTableWidget::AllButtons ^ ObjectsTableWidget::UpdateButton, true, this);
		references_tab->setColumnCount(5);
		references_tab->setHeaderLabel(tr("Col./Expr."), 0);
		references_tab->setHeaderLabel(tr("Table alias"), 1);
		references_tab->setHeaderLabel(tr("Column alias"), 2);
		references_tab->setHeaderLabel(tr("Flags: SF FW AW EX VD"), 3);
		references_tab->setHeaderLabel(tr("Reference alias"), 4);

		vbox=new QVBoxLayout(tabWidget->widget(0));
		vbox->setContentsMargins(GuiUtilsNs::LtMargin,GuiUtilsNs::LtMargin,GuiUtilsNs::LtMargin,GuiUtilsNs::LtMargin);
		vbox->addWidget(references_tab);

		cte_expression_cp=new CodeCompletionWidget(cte_expression_txt, true);

		//Configuring the table objects that stores the triggers and rules
		for(unsigned i=0, tab_id=1; i < sizeof(types)/sizeof(ObjectType); i++, tab_id++)
		{
			tab=new ObjectsTableWidget(ObjectsTableWidget::AllButtons ^
									  (ObjectsTableWidget::UpdateButton  | ObjectsTableWidget::MoveButtons), true, this);

			objects_tab_map[types[i]]=tab;

			grid=new QGridLayout;
			grid->addWidget(tab, 0,0,1,1);
			grid->setContentsMargins(GuiUtilsNs::LtMargin,GuiUtilsNs::LtMargin,GuiUtilsNs::LtMargin,GuiUtilsNs::LtMargin);
			tabWidget->widget(tab_id)->setLayout(grid);

			connect(tab, &ObjectsTableWidget::s_rowsRemoved, this, &ViewWidget::removeObjects);
			connect(tab, &ObjectsTableWidget::s_rowRemoved, this, &ViewWidget::removeObject);
			connect(tab, &ObjectsTableWidget::s_rowAdded, this, &ViewWidget::handleObject);
			connect(tab, &ObjectsTableWidget::s_rowEdited, this, &ViewWidget::handleObject);
			connect(tab, &ObjectsTableWidget::s_rowDuplicated, this, &ViewWidget::duplicateObject);
		}

		objects_tab_map[ObjectType::Trigger]->setColumnCount(6);
		objects_tab_map[ObjectType::Trigger]->setHeaderLabel(tr("Name"), 0);
		objects_tab_map[ObjectType::Trigger]->setHeaderIcon(QPixmap(GuiUtilsNs::getIconPath("uid")),0);
		objects_tab_map[ObjectType::Trigger]->setHeaderLabel(tr("Refer. Table"), 1);
		objects_tab_map[ObjectType::Trigger]->setHeaderIcon(QPixmap(GuiUtilsNs::getIconPath("table")),1);
		objects_tab_map[ObjectType::Trigger]->setHeaderLabel(tr("Firing"), 2);
		objects_tab_map[ObjectType::Trigger]->setHeaderIcon(QPixmap(GuiUtilsNs::getIconPath("trigger")),2);
		objects_tab_map[ObjectType::Trigger]->setHeaderLabel(tr("Events"), 3);
		objects_tab_map[ObjectType::Trigger]->setHeaderLabel(tr("Alias"), 4);
		objects_tab_map[ObjectType::Trigger]->setHeaderLabel(tr("Comment"), 5);

		objects_tab_map[ObjectType::Index]->setColumnCount(4);
		objects_tab_map[ObjectType::Index]->setHeaderLabel(tr("Name"), 0);
		objects_tab_map[ObjectType::Index]->setHeaderIcon(QPixmap(GuiUtilsNs::getIconPath("uid")),0);
		objects_tab_map[ObjectType::Index]->setHeaderLabel(tr("Indexing"), 1);
		objects_tab_map[ObjectType::Index]->setHeaderLabel(tr("Alias"), 2);
		objects_tab_map[ObjectType::Index]->setHeaderLabel(tr("Comment"), 3);

		objects_tab_map[ObjectType::Rule]->setColumnCount(5);
		objects_tab_map[ObjectType::Rule]->setHeaderLabel(tr("Name"), 0);
		objects_tab_map[ObjectType::Rule]->setHeaderIcon(QPixmap(GuiUtilsNs::getIconPath("uid")),0);
		objects_tab_map[ObjectType::Rule]->setHeaderLabel(tr("Execution"), 1);
		objects_tab_map[ObjectType::Rule]->setHeaderLabel(tr("Event"), 2);
		objects_tab_map[ObjectType::Rule]->setHeaderLabel(tr("Alias"), 3);
		objects_tab_map[ObjectType::Rule]->setHeaderLabel(tr("Comment"), 4);


		tablespace_sel->setEnabled(false);
		tablespace_lbl->setEnabled(false);
		configureFormLayout(view_grid, ObjectType::View);

		connect(references_tab, &ObjectsTableWidget::s_rowAdded, this, &ViewWidget::addReference);
		connect(references_tab, &ObjectsTableWidget::s_rowEdited, this, &ViewWidget::editReference);
		connect(references_tab, &ObjectsTableWidget::s_rowDuplicated, this, &ViewWidget::duplicateReference);
		connect(tabWidget, &QTabWidget::currentChanged, this, &ViewWidget::updateCodePreview);

		connect(materialized_rb, &QRadioButton::toggled, with_no_data_chk, &QCheckBox::setEnabled);
		connect(materialized_rb, &QRadioButton::toggled, tablespace_sel, &ObjectSelectorWidget::setEnabled);
		connect(materialized_rb, &QRadioButton::toggled, tablespace_lbl, &QLabel::setEnabled);

		connect(materialized_rb, &QRadioButton::toggled, this, &ViewWidget::updateCodePreview);
		connect(recursive_rb,  &QRadioButton::toggled,  this, &ViewWidget::updateCodePreview);
		connect(with_no_data_chk, &QCheckBox::toggled, this, &ViewWidget::updateCodePreview);
		connect(tablespace_sel, &ObjectSelectorWidget::s_objectSelected, this, &ViewWidget::updateCodePreview);
		connect(tablespace_sel, &ObjectSelectorWidget::s_selectorCleared, this, &ViewWidget::updateCodePreview);
		connect(schema_sel, &ObjectSelectorWidget::s_objectSelected, this, &ViewWidget::updateCodePreview);
		connect(schema_sel, &ObjectSelectorWidget::s_selectorCleared, this, &ViewWidget::updateCodePreview);

		configureTabOrder({ tag_sel, ordinary_rb, recursive_rb, with_no_data_chk, tabWidget });
		setMinimumSize(660, 650);
	}
	catch(Exception &e)
	{
		throw Exception(e.getErrorMessage(),e.getErrorCode(),__PRETTY_FUNCTION__,__FILE__,__LINE__, &e);
	}
}

ObjectsTableWidget *ViewWidget::getObjectTable(ObjectType obj_type)
{
	if(objects_tab_map.count(obj_type) > 0)
		return objects_tab_map[obj_type];

	return nullptr;
}

template<class Class, class WidgetClass>
int ViewWidget::openEditingForm(TableObject *object)
{
	BaseForm editing_form(this);
	WidgetClass *object_wgt=new WidgetClass;
	object_wgt->setAttributes(this->model, this->op_list,
														dynamic_cast<BaseTable *>(this->object),
														dynamic_cast<Class *>(object));
	editing_form.setMainWidget(object_wgt);

	return editing_form.exec();
}

void ViewWidget::handleObject()
{
	ObjectType obj_type=ObjectType::BaseObject;
	TableObject *object=nullptr;
	ObjectsTableWidget *obj_table=nullptr;

	try
	{
		obj_type=getObjectType(sender());
		obj_table=getObjectTable(obj_type);

		if(obj_table->getSelectedRow()>=0)
			object=reinterpret_cast<TableObject *>(obj_table->getRowData(obj_table->getSelectedRow()).value<void *>());

		if(obj_type==ObjectType::Trigger)
			openEditingForm<Trigger,TriggerWidget>(object);
		else if(obj_type==ObjectType::Index)
			openEditingForm<Index,IndexWidget>(object);
		else
			openEditingForm<Rule,RuleWidget>(object);

		listObjects(obj_type);
	}
	catch(Exception &e)
	{
		listObjects(obj_type);
		throw Exception(e.getErrorMessage(),e.getErrorCode(),__PRETTY_FUNCTION__,__FILE__,__LINE__, &e);
	}
}

void ViewWidget::duplicateObject(int curr_row, int new_row)
{
	ObjectType obj_type=ObjectType::BaseObject;
	BaseObject *object=nullptr, *dup_object=nullptr;
	ObjectsTableWidget *obj_table=nullptr;
	View *view = dynamic_cast<View *>(this->object);
	int op_id = -1;

	try
	{
		obj_type=getObjectType(sender());

		//Selects the object table based upon the passed object type
		obj_table=getObjectTable(obj_type);

		//Gets the object reference if there is an item select on table
		if(curr_row >= 0)
			object = reinterpret_cast<BaseObject *>(obj_table->getRowData(curr_row).value<void *>());

		CoreUtilsNs::copyObject(&dup_object, object, obj_type);
		dup_object->setName(CoreUtilsNs::generateUniqueName(dup_object, *view->getObjectList(obj_type), false, "_cp"));

		op_id=op_list->registerObject(dup_object, Operation::ObjCreated, new_row, this->object);

		view->addObject(dup_object);
		view->setModified(true);
		listObjects(obj_type);
	}
	catch(Exception &e)
	{
		//If operation was registered
		if(op_id >= 0)
		{
			op_list->ignoreOperationChain(true);
			op_list->removeLastOperation();
			op_list->ignoreOperationChain(false);
		}

		listObjects(obj_type);
		throw Exception(e.getErrorMessage(),e.getErrorCode(),__PRETTY_FUNCTION__,__FILE__,__LINE__, &e);
	}
}

void ViewWidget::removeObjects()
{
	View *view=nullptr;
	unsigned count, op_count=0, i;
	BaseObject *object=nullptr;
	ObjectType obj_type=getObjectType(sender());

	try
	{
		view=dynamic_cast<View *>(this->object);
		op_count=op_list->getCurrentSize();

		while(view->getObjectCount(obj_type) > 0)
		{
			object=view->getObject(0, obj_type);
			view->removeObject(object);
			op_list->registerObject(object, Operation::ObjRemoved, 0, this->object);
		}
	}
	catch(Exception &e)
	{
		if(op_count < op_list->getCurrentSize())
		{
			count=op_list->getCurrentSize()-op_count;
			op_list->ignoreOperationChain(true);

			for(i=0; i < count; i++)
			{
				op_list->undoOperation();
				op_list->removeLastOperation();
			}

			op_list->ignoreOperationChain(false);
		}

		listObjects(obj_type);
		throw Exception(e.getErrorMessage(),e.getErrorCode(),__PRETTY_FUNCTION__,__FILE__,__LINE__, &e);
	}
}

void ViewWidget::addReference(int row)
{
	openReferenceForm(Reference(), row, false);
}

void ViewWidget::duplicateReference(int orig_row, int new_row)
{
 showReferenceData(references_tab->getRowData(orig_row).value<Reference>(),
									 getReferenceFlag(orig_row), new_row);
}

void ViewWidget::removeObject(int row)
{
	View *view=nullptr;
	BaseObject *object=nullptr;
	ObjectType obj_type=getObjectType(sender());

	try
	{
		view=dynamic_cast<View *>(this->object);
		object=view->getObject(row, obj_type);
		view->removeObject(object);
		op_list->registerObject(object, Operation::ObjRemoved, row, this->object);
	}
	catch(Exception &e)
	{
		listObjects(obj_type);
		throw Exception(e.getErrorMessage(),e.getErrorCode(),__PRETTY_FUNCTION__,__FILE__,__LINE__, &e);
	}
}

ObjectType ViewWidget::getObjectType(QObject *sender)
{
	ObjectType obj_type=ObjectType::BaseObject;

	if(sender)
	{
		std::map<ObjectType, ObjectsTableWidget *>::iterator itr, itr_end;

		itr=objects_tab_map.begin();
		itr_end=objects_tab_map.end();

		while(itr!=itr_end && obj_type==ObjectType::BaseObject)
		{
			if(itr->second==sender)
				obj_type=itr->first;

			itr++;
		}
	}

	return obj_type;
}

void ViewWidget::showObjectData(TableObject *object, int row)
{
	ObjectsTableWidget *tab=nullptr;
	Trigger *trigger=nullptr;
	Rule *rule=nullptr;
	Index *index=nullptr;
	ObjectType obj_type;
	QString str_aux;
	unsigned i;
	EventType events[]={ EventType::OnInsert, EventType::OnDelete,
						 EventType::OnTruncate,	EventType::OnUpdate };

	obj_type=object->getObjectType();
	tab=objects_tab_map[obj_type];

	//Column 0: Object name
	tab->setCellText(object->getName(),row,0);

	if(obj_type==ObjectType::Trigger)
	{
		trigger=dynamic_cast<Trigger *>(object);

		//Column 1: Table referenced by the trigger (constraint trigger)
		tab->clearCellText(row,1);
		if(trigger->getReferencedTable())
			tab->setCellText(trigger->getReferencedTable()->getName(true),row,1);

		//Column 2: Trigger firing type
		tab->setCellText(~trigger->getFiringType(),row,2);

		//Column 3: Events that fires the trigger
		for(i=0; i < sizeof(events)/sizeof(EventType); i++)
		{
			if(trigger->isExecuteOnEvent(events[i]))
				str_aux+=~events[i] + ", ";
		}

		str_aux.remove(str_aux.size()-2, 2);
		tab->setCellText(str_aux ,row,3);
		tab->setCellText(trigger->getAlias(), row, 4);
	}
	else if(obj_type==ObjectType::Rule)
	{
		rule=dynamic_cast<Rule *>(object);

		//Column 1: Rule execution type
		tab->setCellText(~rule->getExecutionType(),row,1);

		//Column 2: Rule event type
		tab->setCellText(~rule->getEventType(),row,2);

		tab->setCellText(rule->getAlias(), row, 3);
	}
	else
	{
		index=dynamic_cast<Index *>(object);

		//Column 1: Indexing type
		tab->setCellText(~index->getIndexingType(),row,1);
		tab->setCellText(index->getAlias(), row, 2);
	}

	tab->setCellText(object->getComment(), row, tab->getColumnCount() - 1);
	tab->setRowData(QVariant::fromValue<void *>(object), row);
}

void ViewWidget::listObjects(ObjectType obj_type)
{
	ObjectsTableWidget *tab=nullptr;
	unsigned count, i;
	View *view=nullptr;

	try
	{
		//Gets the object table related to the object type
		tab=objects_tab_map[obj_type];
		view=dynamic_cast<View *>(this->object);

		tab->blockSignals(true);
		tab->removeRows();

		count=view->getObjectCount(obj_type);
		for(i=0; i < count; i++)
		{
			tab->addRow();
			showObjectData(dynamic_cast<TableObject*>(view->getObject(i, obj_type)), i);
		}
		tab->clearSelection();
		tab->blockSignals(false);
	}
	catch(Exception &e)
	{
		throw Exception(e.getErrorMessage(),e.getErrorCode(),__PRETTY_FUNCTION__,__FILE__,__LINE__, &e);
	}
}

int ViewWidget::openReferenceForm(Reference ref, int row, bool update)
{
	BaseForm editing_form(this);
	ReferenceWidget *ref_wgt=new ReferenceWidget;
	int result = 0;

	editing_form.setMainWidget(ref_wgt);
	editing_form.setButtonConfiguration(Messagebox::OkCancelButtons);

	disconnect(editing_form.apply_ok_btn, &QPushButton::clicked, &editing_form, &BaseForm::accept);
	connect(editing_form.apply_ok_btn, &QPushButton::clicked, ref_wgt, &ReferenceWidget::applyConfiguration);
	connect(ref_wgt, &ReferenceWidget::s_closeRequested, &editing_form, &BaseForm::accept);

	ref_wgt->setAttributes(ref, getReferenceFlag(row), model);
	result = editing_form.exec();
	disconnect(ref_wgt, nullptr, &editing_form, nullptr);

	if(result == QDialog::Accepted)
		showReferenceData(ref_wgt->getReference(), ref_wgt->getReferenceFlags(), row);
	else if(!update)
		references_tab->removeRow(row);

	return result;
}

unsigned ViewWidget::getReferenceFlag(int row)
{
	QString flags_str = references_tab->getCellText(row, 3);
	unsigned ref_flags = 0;

	if(flags_str.isEmpty())
		return 0;

	if(flags_str[4] == '1')
		ref_flags = Reference::SqlViewDef;
	else
	{
		if(flags_str[0] == '1')
			ref_flags |= Reference::SqlSelect;

		if(flags_str[1] == '1')
			ref_flags |= Reference::SqlFrom;

		if(flags_str[2] == '1')
			ref_flags |= Reference::SqlWhere;

		if(flags_str[3] == '1')
			ref_flags |= Reference::SqlEndExpr;
	}

	return ref_flags;
}

void ViewWidget::editReference(int ref_idx)
{
	openReferenceForm(references_tab->getRowData(ref_idx).value<Reference>(), ref_idx, true);
}

void ViewWidget::showReferenceData(Reference refer, unsigned ref_flags, unsigned row)
{
	PhysicalTable *tab=nullptr;
	Column *col=nullptr;
	QString str_aux;
	bool	selec_from = (ref_flags & Reference::SqlSelect) == Reference::SqlSelect,
				from_where = (ref_flags & Reference::SqlFrom) == Reference::SqlFrom,
				after_where = (ref_flags & Reference::SqlWhere) == Reference::SqlWhere,
				end_expr = (ref_flags & Reference::SqlEndExpr) == Reference::SqlEndExpr,
				view_def = (ref_flags & Reference::SqlViewDef) == Reference::SqlViewDef;

	if(refer.getReferenceType()==Reference::ReferColumn)
	{
		tab=refer.getTable();
		col=refer.getColumn();

		/* If the table is allocated but not the column indicates that the reference
		 is to all table columns this way shows a string in format: [SCHEMA].[TABLE].* */
		if(tab && !col)
			references_tab->setCellText(tab->getName(true) + ".*",row,0);
		/* If the table and column are allocated indicates that the reference
		 is to a specific column this way shows a string in format: [SCHEMA].[TABLE].[COLUMN] */
		else
			references_tab->setCellText(tab->getName(true) + "." + col->getName(true),row,0);

		references_tab->setCellText(refer.getAlias(),row,1);

		if(col)
			references_tab->setCellText(refer.getColumnAlias(),row,2);
	}
	else
	{
		references_tab->setCellText(refer.getExpression().simplified(),row,0);
		references_tab->setCellText(refer.getAlias(),row,1);
	}

	//Configures the string that denotes the SQL application for the reference
	str_aux+=(selec_from ? "1" : "0");
	str_aux+=(from_where ? "1" : "0");
	str_aux+=(after_where ? "1" : "0");
	str_aux+=(end_expr ? "1" : "0");
	str_aux+=(view_def ? "1" : "0");
	references_tab->setCellText(str_aux, row, 3);

	references_tab->setCellText(refer.getReferenceAlias(), row, 4);

	refer.setDefinitionExpression(view_def);
	references_tab->setRowData(QVariant::fromValue<Reference>(refer), row);
}

void ViewWidget::updateCodePreview()
{
	try
	{
		if(tabWidget->currentIndex()==tabWidget->count()-1)
		{
			View aux_view;
			Reference refer;
			QString str_aux;
			unsigned i, count, i1;
			Reference::SqlType expr_type[]={
				Reference::SqlSelect,
				Reference::SqlFrom,
				Reference::SqlWhere,
				Reference::SqlEndExpr,
				Reference::SqlViewDef
			};

			aux_view.BaseObject::setName(name_edt->text().toUtf8());
			aux_view.BaseObject::setSchema(schema_sel->getSelectedObject());
			aux_view.setTablespace(tablespace_sel->getSelectedObject());

			aux_view.setCommomTableExpression(cte_expression_txt->toPlainText().toUtf8());
			aux_view.setMaterialized(materialized_rb->isChecked());
			aux_view.setRecursive(recursive_rb->isChecked());
			aux_view.setWithNoData(with_no_data_chk->isChecked());

			count=references_tab->getRowCount();
			for(i=0; i < count; i++)
			{
				refer=references_tab->getRowData(i).value<Reference>();

				//Get the SQL application string for the current reference
				str_aux=references_tab->getCellText(i,3);

				for(i1=0; i1 < 5; i1++)
				{
					if(str_aux[i1]=='1')
						aux_view.addReference(refer, expr_type[i1]);
				}
			}

			code_txt->setPlainText(aux_view.getSourceCode(SchemaParser::SqlCode));
		}
	}
	catch(Exception &e)
	{
		//In case of error no code is outputed, showing a error message in the code preview widget
		QString str_aux=tr("/* Could not generate the SQL code. Make sure all attributes are correctly filled! ");
		str_aux+=QString("\n\n>> Returned error(s): \n\n%1*/").arg(e.getExceptionsText());
		code_txt->setPlainText(str_aux);
	}
}

void ViewWidget::setAttributes(DatabaseModel *model, OperationList *op_list, Schema *schema, View *view, double px, double py)
{
	unsigned i, count, ref_flags = 0;
	Reference refer;

	if(!view)
	{
		view=new View;

		if(schema)
			view->setSchema(schema);

		/* Sets the 'new_object' flag as true indicating that the alocated table must be treated
			 as a recently created object */
		this->new_object=true;
	}

	BaseObjectWidget::setAttributes(model,op_list, view, schema, px, py);

	materialized_rb->setChecked(view->isMaterialized());
	recursive_rb->setChecked(view->isRecursive());
	with_no_data_chk->setChecked(view->isWithNoData());

	cte_expression_cp->configureCompletion(model, cte_expression_hl);

	op_list->startOperationChain();
	operation_count=op_list->getCurrentSize();

	tag_sel->setModel(this->model);
	tag_sel->setSelectedObject(view->getTag());

	cte_expression_txt->setPlainText(view->getCommomTableExpression());

	count=view->getReferenceCount();
	references_tab->blockSignals(true);

	for(i=0; i < count; i++)
	{
		references_tab->addRow();

		ref_flags = 0;
		refer=view->getReference(i);

		if(view->getReferenceIndex(refer, Reference::SqlViewDef) >= 0)
			ref_flags = Reference::SqlViewDef;

		if(view->getReferenceIndex(refer, Reference::SqlSelect) >= 0)
			ref_flags |= Reference::SqlSelect;

		if(view->getReferenceIndex(refer, Reference::SqlFrom) >= 0)
			ref_flags |= Reference::SqlFrom;

		if(view->getReferenceIndex(refer, Reference::SqlWhere) >= 0)
			ref_flags |= Reference::SqlWhere;

		if(view->getReferenceIndex(refer, Reference::SqlEndExpr) >= 0)
			ref_flags |= Reference::SqlEndExpr;

		showReferenceData(refer, ref_flags, i);
	}

	references_tab->blockSignals(false);
	references_tab->clearSelection();

	listObjects(ObjectType::Trigger);
	listObjects(ObjectType::Rule);
	listObjects(ObjectType::Index);
}

void ViewWidget::applyConfiguration()
{
	try
	{
		View *view=nullptr;
		ObjectType types[]={ ObjectType::Trigger, ObjectType::Rule, ObjectType::Index };
		Reference::SqlType expr_type[]={ Reference::SqlSelect,
																		 Reference::SqlFrom,
																		 Reference::SqlWhere,
																		 Reference::SqlEndExpr,
																		 Reference::SqlViewDef};
		Reference refer;
		QString str_aux;

		if(!this->new_object)
			op_list->registerObject(this->object, Operation::ObjModified);
		else
			registerNewObject();

		BaseObjectWidget::applyConfiguration();

		view=dynamic_cast<View *>(this->object);
		view->removeObjects();
		view->removeReferences();
		view->setMaterialized(materialized_rb->isChecked());
		view->setRecursive(recursive_rb->isChecked());
		view->setWithNoData(with_no_data_chk->isChecked());
		view->setCommomTableExpression(cte_expression_txt->toPlainText().toUtf8());
		view->setTag(dynamic_cast<Tag *>(tag_sel->getSelectedObject()));

		for(unsigned i=0; i < references_tab->getRowCount(); i++)
		{
			refer=references_tab->getRowData(i).value<Reference>();

			//Get the SQL application string for the current reference
			str_aux=references_tab->getCellText(i, 3);
			for(unsigned i=0; i < sizeof(expr_type)/sizeof(unsigned); i++)
			{
				if(str_aux[i]=='1')
					view->addReference(refer, expr_type[i]);
			}
		}

		//Adds the auxiliary view objects into configured view
		for(unsigned type_id=0; type_id < sizeof(types)/sizeof(ObjectType); type_id++)
		{
			for(unsigned i=0; i < objects_tab_map[types[type_id]]->getRowCount(); i++)
				view->addObject(reinterpret_cast<TableObject *>(objects_tab_map[types[type_id]]->getRowData(i).value<void *>()));
		}

		op_list->finishOperationChain();
		this->model->updateViewRelationships(view);
		finishConfiguration();
	}
	catch(Exception &e)
	{
		throw Exception(e.getErrorMessage(),e.getErrorCode(),__PRETTY_FUNCTION__,__FILE__,__LINE__, &e);
	}
}

void ViewWidget::cancelConfiguration()
{
	BaseObjectWidget::cancelChainedOperation();
}
